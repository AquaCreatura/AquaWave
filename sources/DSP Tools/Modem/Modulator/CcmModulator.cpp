#include "CcmModulator.h"

#include "../../basic/filter_tools.h"

#include <algorithm>

namespace
{
	// Формирующий фильтр RRC: коэффициент сглаживания и протяжённость (в символах).
	constexpr double kRrcRolloff = 0.35;
	constexpr int    kRrcSpanSymbols = 8;
}

namespace aq_mod
{

	// ===== FirStage =====

	bool CcmModulator::FirStage::Init(const std::vector<Ipp32fc>& taps)
	{
		Free();

		len = static_cast<int>(taps.size());
		if (len < 2)
			return false;

		int spec_size = 0;
		int buf_size = 0;
		if (ippsFIRSRGetSize(len, ipp32fc, &spec_size, &buf_size) != ippStsNoErr)
			return false;

		spec = reinterpret_cast<IppsFIRSpec_32fc*>(ippsMalloc_8u(spec_size));
		buf = ippsMalloc_8u(buf_size);
		if (!spec || !buf)
		{
			Free();
			return false;
		}

		if (ippsFIRSRInit_32fc(taps.data(), len, ippAlgAuto, spec) != ippStsNoErr)
		{
			Free();
			return false;
		}

		delay.assign(len, Ipp32fc{ 0.0f, 0.0f });
		return true;
	}

	void CcmModulator::FirStage::Free()
	{
		if (spec)
		{
			ippsFree(spec);
			spec = nullptr;
		}
		if (buf)
		{
			ippsFree(buf);
			buf = nullptr;
		}
		delay.clear();
		len = 0;
	}

	void CcmModulator::FirStage::Reset()
	{
		std::fill(delay.begin(), delay.end(), Ipp32fc{ 0.0f, 0.0f });
	}

	bool CcmModulator::FirStage::Process(const Ipp32fc* src, Ipp32fc* dst, int count)
	{
		if (!spec || len <= 0)
		{
			std::copy(src, src + count, dst);
			return true;
		}
		return ippsFIRSR_32fc(src, dst, count, spec, delay.data(), delay.data(), buf) == ippStsNoErr;
	}

	// ===== CcmModulator =====

	CcmModulator::CcmModulator()
	{
	}

	CcmModulator::~CcmModulator()
	{
		rrc_.Free();
		after_.Free();
	}

	bool CcmModulator::Init(const char* modulation, int upsample)
	{
		upsample_ = std::max(1, upsample);
		bits_per_symbol_ = 0;
		lookup_.clear();

		const std::vector<Ipp32fc> pivots = aq_demod::CalculateModulationPivots(modulation);
		if (pivots.size() < 2)
			return false;

		BuildLookupTable(pivots);
		if (lookup_.empty())
			return false;

		if (!BuildRrcFilter())
			return false;

		// Дополнительный ФНЧ строится только если включён.
		after_.Free();
		if (after_filter_enabled_ && !BuildAfterFilter())
			return false;

		Reset();
		return true;
	}

	void CcmModulator::BuildLookupTable(const std::vector<Ipp32fc>& pivots)
	{
		const int point_count = static_cast<int>(pivots.size());

		bits_per_symbol_ = 0;
		while ((1 << bits_per_symbol_) < point_count)
			++bits_per_symbol_;

		// Only power-of-two constellations can be mapped to whole bit groups.
		if ((1 << bits_per_symbol_) != point_count)
			return;

		const std::vector<std::string> combinations =
			aq_demod::GetTestingManipVec(point_count);
		if (static_cast<int>(combinations.size()) != point_count)
			return;

		// Lookup table: each bit combination gets its own IQ point. The order of
		// the combinations matches the order of the constellation points, so the
		// bit string is decoded into an index and the matching point is stored.
		lookup_.assign(point_count, Ipp32fc{ 0.0f, 0.0f });
		for (int i = 0; i < point_count; ++i)
		{
			int index = 0;
			for (char symbol : combinations[i])
				index = (index << 1) | (symbol == '1' ? 1 : 0);
			lookup_[index] = pivots[i];
		}
	}

	bool CcmModulator::BuildRrcFilter()
	{
		// Формирующий фильтр - корневой приподнятый косинус (RRC). Длина фильтра
		// берётся нечётной, чтобы центральный отвод приходился ровно на отсчёт
		// (span символов в обе стороны). Нормировку на единичное усиление по DC
		// выполняет генератор коэффициентов.
		const int filter_len = kRrcSpanSymbols * upsample_ + 1;

		std::vector<Ipp32fc> taps;
		if (!aqua_dsp_tools::GenerateRrcKoeffs(kRrcRolloff, upsample_, filter_len, taps))
			return false;

		return rrc_.Init(taps);
	}

	bool CcmModulator::BuildAfterFilter()
	{
		// Обычный ФНЧ (оконный sinc) с частотой среза на символьной скорости:
		// его нули приходятся на символьные отсчёты.
		const double cutoff = std::min(1.0 / static_cast<double>(upsample_), 0.45);
		int filter_len = aqua_dsp_tools::GetFirLenPow2(1.0, std::max(0.95, cutoff));
		filter_len = std::min(std::max(16, filter_len), 1024);

		std::vector<Ipp32fc> taps(filter_len);
		if (!aqua_dsp_tools::GenerateWindowKoeffs(cutoff, filter_len, ippWinBlackman, taps))
			return false;

		return after_.Init(taps);
	}

	void CcmModulator::EnableAfterFilter(bool enabled)
	{
		after_filter_enabled_ = enabled;

		// До Init() фильтр построится (или нет) по флагу; после Init() меняем
		// состояние сразу.
		if (lookup_.empty())
			return;

		if (enabled)
		{
			if (after_.len <= 0)
				BuildAfterFilter();
		}
		else
		{
			after_.Free();
		}
	}

	void CcmModulator::Reset()
	{
		rrc_.Reset();
		after_.Reset();
		symbols_.clear();
		upsampled_.clear();
		shaped_.clear();
	}

	bool CcmModulator::Process(const std::vector<uint8_t>& bits, std::vector<Ipp32fc>& out_iq)
	{
		out_iq.clear();
		if (lookup_.empty() || bits_per_symbol_ <= 0)
			return false;

		const int bits_per_point = bits_per_symbol_;
		const size_t point_count = bits.size() / static_cast<size_t>(bits_per_point);
		if (point_count == 0)
			return true;

		// Bit stream -> constellation points.
		symbols_.resize(point_count);
		for (size_t i = 0; i < point_count; ++i)
		{
			int index = 0;
			for (int bit = 0; bit < bits_per_point; ++bit)
				index = (index << 1) | (bits[i * bits_per_point + bit] & 1u);
			symbols_[i] = lookup_[index];
		}

		// Upsampling by zero insertion.
		const Ipp32fc* source = symbols_.data();
		int source_len = static_cast<int>(point_count);
		if (upsample_ > 1)
		{
			upsampled_.resize(point_count * static_cast<size_t>(upsample_));
			int dst_len = 0;
			int phase = 0;
			if (ippsSampleUp_32fc(symbols_.data(), static_cast<int>(point_count),
				upsampled_.data(), &dst_len, upsample_, &phase) != ippStsNoErr)
				return false;
			upsampled_.resize(dst_len);
			source = upsampled_.data();
			source_len = dst_len;
		}

		// Формирующий RRC, затем (при включении) дополнительный ФНЧ. Пустой
		// after_ просто копирует данные, поэтому отдельная ветка не нужна.
		shaped_.resize(source_len);
		if (!rrc_.Process(source, shaped_.data(), source_len))
			return false;

		out_iq.resize(source_len);
		if (!after_.Process(shaped_.data(), out_iq.data(), source_len))
		{
			out_iq.clear();
			return false;
		}

		return true;
	}

	std::vector<uint8_t> CcmModulator::BytesToBits(const std::vector<uint8_t>& bytes)
	{
		std::vector<uint8_t> bits;
		bits.reserve(bytes.size() * 8);
		for (uint8_t byte : bytes)
		{
			for (int bit = 7; bit >= 0; --bit)
				bits.push_back((byte >> bit) & 1u);
		}
		return bits;
	}

}
