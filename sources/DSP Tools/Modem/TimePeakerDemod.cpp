#include "TimePeakerDemod.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace aq_demod
{

	TimePeakerDemod::TimePeakerDemod(int upsample_koeff)
		: upsample_koeff_(upsample_koeff),
		w_(static_cast<double>(upsample_koeff)),
		pos_(static_cast<double>(upsample_koeff)),
		gain_mu_(0.03),
		gain_omega_(0.000225),
		omega_min_(0.9 * upsample_koeff),
		omega_max_(1.1 * upsample_koeff),
		power_(0.0),
		prev_symb_{ 0.0f, 0.0f },
		have_prev_symb_(false)
	{
		if (upsample_koeff_ != 2 && upsample_koeff_ != 4)
			throw std::invalid_argument("TimePeakerDemod supports only 2 or 4 samples/symbol");

		history_.reserve(4);
	}

	TimePeakerDemod::~TimePeakerDemod()
	{
	}

	void TimePeakerDemod::Reset()
	{
		w_ = static_cast<double>(upsample_koeff_);
		pos_ = static_cast<double>(upsample_koeff_);
		power_ = 0.0;
		prev_symb_ = { 0.0f, 0.0f };
		have_prev_symb_ = false;
		history_.clear();
	}

	Ipp32fc TimePeakerDemod::GetSample(const std::vector<Ipp32fc>& signal, int index) const
	{
		if (index >= 0)
			return signal[index];

		return history_[static_cast<int>(history_.size()) + index];
	}

	Ipp32fc TimePeakerDemod::Interpolate(const std::vector<Ipp32fc>& signal, double pos) const
	{
		const int index = static_cast<int>(std::floor(pos));
		const double mu = pos - index;

		//  убическа€ интерпол€ци€ уменьшает ошибку от дробной позиции.
		const double c0 = -mu * (mu - 1.0) * (mu - 2.0) / 6.0;
		const double c1 = (mu + 1.0) * (mu - 1.0) * (mu - 2.0) / 2.0;
		const double c2 = -(mu + 1.0) * mu * (mu - 2.0) / 2.0;
		const double c3 = (mu + 1.0) * mu * (mu - 1.0) / 6.0;

		const Ipp32fc s0 = GetSample(signal, index - 1);
		const Ipp32fc s1 = GetSample(signal, index);
		const Ipp32fc s2 = GetSample(signal, index + 1);
		const Ipp32fc s3 = GetSample(signal, index + 2);

		return {
			static_cast<Ipp32f>(c0 * s0.re + c1 * s1.re + c2 * s2.re + c3 * s3.re),
			static_cast<Ipp32f>(c0 * s0.im + c1 * s1.im + c2 * s2.im + c3 * s3.im)
		};
	}

	double TimePeakerDemod::GetError(const Ipp32fc& prev, const Ipp32fc& mid, const Ipp32fc& cur)
	{
		// Gardner использует середину символа и разность соседних символов.
		const double dr = static_cast<double>(prev.re) - cur.re;
		const double di = static_cast<double>(prev.im) - cur.im;

		const double error = static_cast<double>(mid.re) * dr + static_cast<double>(mid.im) * di;

		const double cur_power = static_cast<double>(cur.re) * cur.re + static_cast<double>(cur.im) * cur.im;

		constexpr double power_alpha = 0.01;

		if (power_ <= 0.0)
			power_ = cur_power;
		else
			power_ += power_alpha * (cur_power - power_);

		if (power_ > 1.e-12)
			return error / power_;

		return error;
	}

	void TimePeakerDemod::UpdateLoop(double error)
	{
		// mu корректирует timing phase, omega отслеживает clock drift.
		w_ += gain_omega_ * error;
		w_ = std::max(omega_min_, std::min(w_, omega_max_));
		pos_ += w_ + gain_mu_ * error;
	}

	void TimePeakerDemod::UpdateHistory(const std::vector<Ipp32fc>& signal)
	{
		const std::size_t count = std::min<std::size_t>(4, signal.size());

		history_.assign(signal.end() - count, signal.end());
	}

	bool TimePeakerDemod::Process(const std::vector<Ipp32fc>& passed, std::vector<Ipp32fc>& out_samples)
	{
		if (passed.empty())
			return true;

		out_samples.reserve(out_samples.size() + passed.size() / upsample_koeff_ + 1);

		while (pos_ + 2.0 < static_cast<double>(passed.size())) {
			const double mid_pos = pos_ - 0.5 * w_;
			const Ipp32fc cur = Interpolate(passed, pos_);
			const Ipp32fc mid = Interpolate(passed, mid_pos);

			if (have_prev_symb_) {
				const double error = GetError(prev_symb_, mid, cur);
				UpdateLoop(error);
			}

			prev_symb_ = cur;
			have_prev_symb_ = true;
			out_samples.push_back(cur);
		}

		UpdateHistory(passed);
		pos_ -= static_cast<double>(passed.size());

		return true;
	}

} // namespace aq_demod