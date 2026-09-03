#include "ContinuousDemodulator.h"

#include <algorithm>
#include <cmath>
#include <limits>

using namespace aq_demod;

namespace
{
	const double PI = 3.14159265358979323846;
	const double TWO_PI = 2.0 * PI;

	const double MIN_PLL_BW = TWO_PI / 200.0;
	const double MAX_PLL_BW = TWO_PI / 100.0;
}

ContinuousDemodulator::ContinuousDemodulator()
{
}

ContinuousDemodulator::~ContinuousDemodulator()
{
}

bool ContinuousDemodulator::Init(const char* modulation, int upsample_passed)
{
	pivots_ = CalculateModulationPivots(modulation);

	if (pivots_.size() < 2)
		return false;

	upsample_passed_ = upsample_passed;

	target_power_ = 0.0;

	for (const Ipp32fc& pivot : pivots_)
		target_power_ += pivot.re * pivot.re + pivot.im * pivot.im;

	target_power_ /= pivots_.size();
	signal_power_ = 0.0;
	SetPllSpeed(1.0);
	Reset();

	return true;
}

void ContinuousDemodulator::Reset()
{
	phase_ = 0.0;
	freq_offset_ = 0.0;

	agc_power_ = target_power_;

	signal_power_ = 0.0;
	error_power_ = 0.0;
	snr_db_ = 0.0;
}

void ContinuousDemodulator::SetPllSpeed(double speed)
{
	speed = std::max(0.0, std::min(1.0, speed));
	loop_bw_ = MIN_PLL_BW + speed * (MAX_PLL_BW - MIN_PLL_BW);
	UpdateLoopCoefficients();
}

void ContinuousDemodulator::UpdateLoopCoefficients()
{
	const double denom = 1.0 + 2.0 * damping_ * loop_bw_ + loop_bw_ * loop_bw_;

	pll_freq_ = 1.e-2; (4.0 * loop_bw_ * loop_bw_ / denom) ; //Freq
	pll_phase_ = 1.e-3;  (4.0 * damping_ * loop_bw_ / denom) ; // Phase


}

bool ContinuousDemodulator::SynchroniseIQ(
	const std::vector<Ipp32fc>& passed_iq,
	std::vector<Ipp32fc>& synced_iq)
{
	synced_iq.clear();

	ted_man_.Process(passed_iq, synced_iq);

	if ( agc_enabled_)
		ApplyAGC(synced_iq);


	error_power_ = 0.0;

	for (Ipp32fc& sample : synced_iq) {

		// NCO derotates the current symbol using the loop state.
		const Ipp32fc corrected = CorrectPhase(sample);

		double phase_error = 0.0;
		const Ipp32fc decision = GetDecision(corrected, phase_error);

		// Use the current decision to update the second-order loop.
		freq_offset_ += pll_freq_ * phase_error;

		if (freq_offset_ > max_freq_offset_)
			freq_offset_ = 0 * max_freq_offset_;
		if (freq_offset_ < -max_freq_offset_)
			freq_offset_ = 0 * -max_freq_offset_;

		phase_ += freq_offset_ + pll_phase_ * phase_error;

		sample = corrected;

		const double error_re = corrected.re - decision.re;
		const double error_im = corrected.im - decision.im;

		error_power_ += error_re * error_re + error_im * error_im;
		signal_power_ += decision.re * decision.re + decision.im * decision.im;
	}

	if (!synced_iq.empty() && error_power_ > 0.0) {
		snr_db_ = 10.0 * std::log10(signal_power_ / error_power_);
	}

	return true;
}

void ContinuousDemodulator::ApplyAGC(std::vector<Ipp32fc>& signal)
{
	for (Ipp32fc& sample : signal) {

		const double power = sample.re * sample.re + sample.im * sample.im;

		agc_power_ = (1.0 - agc_alpha_) * agc_power_ + agc_alpha_ * power;

		const double gain = std::sqrt(target_power_ / agc_power_);

		sample.re *= static_cast<Ipp32f>(gain);
		sample.im *= static_cast<Ipp32f>(gain);
	}
}

Ipp32fc ContinuousDemodulator::CorrectPhase(const Ipp32fc& sample) const
{
	const float c = std::cos(static_cast<float>(phase_));
	const float s = std::sin(static_cast<float>(phase_));

	return {
		sample.re * c - sample.im * s,
		sample.re * s + sample.im * c
	};
}

Ipp32fc ContinuousDemodulator::GetDecision(
	const Ipp32fc& sample,
	double& phase_error) const
{
	double min_distance = std::numeric_limits<double>::max();
	Ipp32fc decision = pivots_[0];

	for (const Ipp32fc& pivot : pivots_) {

		const double re = sample.re - pivot.re;
		const double im = sample.im - pivot.im;
		const double distance = re * re + im * im;

		if (distance < min_distance) {
			min_distance = distance;
			decision = pivot;
		}
	}

	// GNU Radio uses -arg(sample * conj(decision)) as the detector error.
	const double real = sample.re * decision.re + sample.im * decision.im;
	const double imag = sample.im * decision.re - sample.re * decision.im;

	phase_error = -std::atan2(imag, real);

	return decision;
}