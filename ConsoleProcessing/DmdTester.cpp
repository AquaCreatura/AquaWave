#define NOMINMAX

#include "DmdTester.h"

#include "DSP Tools/Modem/Demodulation/ContinuousDemodulator.h"
#include "DSP Tools/Resampler/ResamplerMan.h"
#include "Arks/File Source/helpers/FileReadCore.h"
#include "special_defs/file_souce_defs.h"
#include "Utilities/file_helpers.h"
#include "Utilities/parse_tools.h"

#include <QElapsedTimer>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace
{
	// The demodulator works at 4 samples per symbol.
	constexpr int kUpsample = 4;
	constexpr double kTwoPi = 2.0 * M_PI;

	// File read block size (in samples).
	constexpr size_t kReadBlockSamples = 1u << 16;

	// One statistics line: percent processed, SNR, frequency offset, symbol rate.
	void PrintStats(QTextStream& out, int percent,
		const aq_demod::ContinuousDemodulator& demodulator, int64_t nominal_symbol_rate_hz)
	{
		const double snr_db = demodulator.GetLastSNR();
		const double freq_offset_hz = demodulator.GetFreqOffset() / kTwoPi * nominal_symbol_rate_hz;
		const double ratio = demodulator.GetSymbolRateRatio();
		const double symbol_rate_hz = (ratio > 1.e-6)
			? nominal_symbol_rate_hz / ratio
			: static_cast<double>(nominal_symbol_rate_hz);

		out << QString("Progress: %1%   SNR = %2 dB   dFc = %3 Hz   Bd = %4 Hz")
			.arg(percent, 3)
			.arg(snr_db, 7, 'f', 2)
			.arg(freq_offset_hz, 10, 'f', 1)
			.arg(symbol_rate_hz, 12, 'f', 1)
			<< "\n";
		out.flush();
	}

	// Resolves the file carrier and sample rate: the given values take priority,
	// otherwise they are parsed from the file name (e.g. "... 11081.434200MHz
	// 7000.000KHz.pcm"). An explicit --file_fc 0 is valid (baseband) and must not be
	// treated as missing.
	void ResolveFileRates(const console::DmdParams& params, int64_t& file_carrier_hz,
		int64_t& file_samplerate_hz)
	{
		const bool have_carrier = static_cast<bool>(params.file_carrier_hz);
		const bool have_samplerate = static_cast<bool>(params.file_samplerate_hz);

		file_carrier_hz = params.file_carrier_hz.value_or(0);
		file_samplerate_hz = params.file_samplerate_hz.value_or(0);

		if (have_carrier && have_samplerate)
			return;

		const QByteArray file_path_bytes = params.input_path.toLocal8Bit();
		int64_t parsed_carrier_hz = 0;
		int64_t parsed_samplerate_hz = 0;
		aqua_parse_tools::parse_filename(file_path_bytes.constData(), parsed_carrier_hz, parsed_samplerate_hz);

		if (!have_carrier) file_carrier_hz = parsed_carrier_hz;
		if (!have_samplerate) file_samplerate_hz = parsed_samplerate_hz;
	}

	// Converts a raw block to complex float samples according to the file data type.
	// Returns the number of complex samples converted.
	size_t ConvertBlock(const std::vector<uint8_t>& read_bytes, IppDataType data_type,
		std::vector<Ipp32fc>& input_block)
	{
		const size_t samples_read = read_bytes.size() / file_source::GetSampleSize(data_type);
		input_block.resize(samples_read);
		if (samples_read == 0)
			return 0;

		if (data_type == IppDataType::ipp16sc)
		{
			ippsConvert_16s32f(reinterpret_cast<const Ipp16s*>(read_bytes.data()),
				reinterpret_cast<Ipp32f*>(input_block.data()),
				static_cast<int>(samples_read * 2));
		}
		else
		{
			std::memcpy(input_block.data(), read_bytes.data(), samples_read * sizeof(Ipp32fc));
		}
		return samples_read;
	}
}

console::LoopRanges console::DmdTester::GetLoopRanges()
{
	aq_demod::ContinuousDemodulator probe;
	LoopRanges loops;
	loops.pll = probe.GetPllSpeedLimits();
	loops.tll = probe.GetTimingSpeedLimits();
	loops.eq = probe.GetEqualiserSpeedLimits();
	loops.pll_default = probe.GetPllSpeedDefault();
	loops.tll_default = probe.GetTimingSpeedDefault();
	loops.eq_default = probe.GetEqualiserSpeedDefault();
	loops.eq_taps_default = probe.GetEqualiserTapsDefault();
	return loops;
}

int console::DmdTester::RunDemodulation(const DmdParams& params, QTextStream& out)
{
	const QByteArray file_path_bytes = params.input_path.toLocal8Bit();

	int64_t file_carrier_hz = 0;
	int64_t file_samplerate_hz = 0;
	ResolveFileRates(params, file_carrier_hz, file_samplerate_hz);
	if (file_samplerate_hz <= 0)
	{
		out << "Error: cannot determine the file sample rate. Specify --file_fs explicitly.\n";
		out.flush();
		return 1;
	}

	// File description: the sample type is detected automatically (int16 or float32).
	aqua::SourceDescription file_descr;
	file_descr.file_name_ = params.input_path;
	file_descr.carrier_hz = file_carrier_hz;
	file_descr.samplerate_hz = file_samplerate_hz;
	file_descr.data_type_ = FsHelper::IsFile32Float(file_path_bytes.constData())
		? IppDataType::ipp32fc
		: IppDataType::ipp16sc;
	file_descr.bw_ratio_ = 0.9;
	file_descr.is_signal = true;
	file_descr.first_sample_offset = 0;

	const int64_t file_bytes = FsHelper::GetFileSize(file_path_bytes.constData());
	const size_t sample_size = file_source::GetSampleSize(file_descr.data_type_);
	if (file_bytes <= 0 || sample_size == 0)
	{
		out << "Error: cannot determine the size of \"" << params.input_path << "\".\n";
		out.flush();
		return 1;
	}
	file_descr.count_of_samples = file_bytes / static_cast<int64_t>(sample_size);

	file_source::StreamReader reader;
	if (!reader.SetFileParams(file_descr))
	{
		out << "Error: cannot open \"" << params.input_path << "\".\n";
		out.flush();
		return 1;
	}
	if (!reader.InitStartEndRatio({ 0.0, 1.0 }))
	{
		out << "Error: cannot initialize file reading.\n";
		out.flush();
		return 1;
	}

	// Resampler: shift the selected carrier to zero and bring the file to 4 samples
	// per symbol relative to the symbol rate.
	aqua_resampler::ResamplerManager resampler;
	resampler.SetBaseParams(file_carrier_hz, file_samplerate_hz);
	// SetTargetParams may adjust the target sample rate, so it is a mutable local.
	int64_t demod_samplerate_hz =
		static_cast<int64_t>(std::llround(params.demod_symbol_rate_hz * kUpsample));
	const int64_t demod_bandwidth_hz = params.demod_symbol_rate_hz;
	if (!resampler.SetTargetParams(params.demod_carrier_hz, demod_samplerate_hz, demod_bandwidth_hz))
	{
		out << "Error: cannot configure the resampler (check carriers and rates).\n";
		out.flush();
		return 1;
	}

	// Demodulator.
	aq_demod::ContinuousDemodulator demodulator;
	const QByteArray modulation = params.modulation.toLatin1();
	if (!demodulator.Init(modulation.constData(), kUpsample))
	{
		out << "Error: unknown demodulation type \"" << params.modulation << "\".\n";
		out.flush();
		return 1;
	}

	// Loop-speed defaults depend on the modulation type: an omitted command line
	// value falls back to the optimal set for this modulation.
	const aq_demod::DmdSliderStructure setup = aq_demod::GetDefaultSliderSetup(modulation.constData());
	const double pll_speed = params.pll_speed.value_or(setup.pll);
	const double tll_speed = params.tll_speed.value_or(setup.tll);
	const double eq_speed = params.eq_speed.value_or(setup.eq_koeff);

	// The equaliser algorithm also depends on the modulation: default AUTO (blind
	// MMA first, then decision-directed LMS after lock).
	const QString eq_alg = params.eq_alg.isEmpty()
		? QString::fromLatin1(setup.eq_alg)
		: params.eq_alg;

	// Set the length before the speed: the NLMS step depends on the tap count.
	if (params.eq_taps > 0) demodulator.SetEqualiserTaps(params.eq_taps);
	demodulator.SetPllSpeed(pll_speed);
	demodulator.SetTimingSpeed(tll_speed);
	demodulator.SetEqualiserSpeed(eq_speed);
	demodulator.SetEqualiserAlgorithm(eq_alg.toLatin1().constData());

	// Header with the run parameters.
	out << "=== AquaWave console demodulation ===\n";
	out << "File:        " << params.input_path << "\n";
	out << "Type:        " << params.modulation << "\n";
	out << "Fc file:     " << file_carrier_hz << " Hz\n";
	out << "Fs file:     " << file_samplerate_hz << " Hz\n";
	out << "Fc demod:    " << params.demod_carrier_hz << " Hz\n";
	out << "Bd demod:    " << params.demod_symbol_rate_hz << " Hz\n";
	out << "Eq alg:      " << eq_alg << "\n";
	out << "Eq taps:     " << demodulator.GetEqualiserTaps() << "\n";
	out << "PLL/TLL/EQ:  " << QString::number(demodulator.GetPllSpeed(), 'g', 4)
		<< " / " << QString::number(demodulator.GetTimingSpeed(), 'g', 4)
		<< " / " << QString::number(demodulator.GetEqualiserSpeed(), 'g', 4) << "\n";
	out << "Demod Fs:    " << demod_samplerate_hz << " Hz (" << kUpsample << " samples/symbol)\n";
	out << "Samples:     " << file_descr.count_of_samples << "\n";
	out << "-------------------------------------------------------------------------------\n";
	out.flush();

	const int64_t total_samples = file_descr.count_of_samples;
	int64_t processed_samples = 0;
	int last_percent = -1;
	double settled_snr = 0.0;
	bool has_settled_snr = false;

	QElapsedTimer timer;
	timer.start();

	std::vector<uint8_t> read_bytes;
	std::vector<Ipp32fc> input_block;
	std::vector<Ipp32fc> synced;

	while (reader.IsReadStreamAvailable())
	{
		if (!reader.ReadStream(read_bytes, kReadBlockSamples))
			break;

		const size_t samples_read = ConvertBlock(read_bytes, file_descr.data_type_, input_block);
		if (samples_read == 0)
			break;

		// Carrier shift and conversion to 4 samples per symbol.
		if (resampler.ProcessBlock(input_block.data(), input_block.size()))
		{
			std::vector<Ipp32fc>& resampled = resampler.GetProcessedData();
			if (!resampled.empty())
				demodulator.SynchroniseIQ(resampled, synced);
		}

		processed_samples += static_cast<int64_t>(samples_read);

		// The settled SNR is accumulated after the first 20% of the file, once the
		// loops have locked.
		if (processed_samples > total_samples / 5)
		{
			const double snr = demodulator.GetLastSNR();
			if (std::isfinite(snr))
			{
				settled_snr = has_settled_snr ? (settled_snr * 0.9 + snr * 0.1) : snr;
				has_settled_snr = true;
			}
		}

		const int percent = static_cast<int>(processed_samples * 100 / total_samples);
		if (percent != last_percent)
		{
			last_percent = percent;
			PrintStats(out, percent, demodulator, params.demod_symbol_rate_hz);
		}
	}

	// Final statistics.
	out << "-------------------------------------------------------------------------------\n";
	out << "Processed:   " << processed_samples << " / " << total_samples << " samples\n";
	out << "Time:        " << QString::number(timer.elapsed() / 1000.0, 'f', 2) << " s\n";
	PrintStats(out, 100, demodulator, params.demod_symbol_rate_hz);
	if (has_settled_snr)
		out << "SNR (settled, average): " << QString::number(settled_snr, 'f', 2) << " dB\n";
	if (demodulator.IsEqualiserWarmUp())
		out << "Eq warm-up:  "
			<< (demodulator.IsEqualiserLocked() ? "locked (LMS)" : "searching (MMA)")
			<< "\n";
	out.flush();

	return 0;
}
