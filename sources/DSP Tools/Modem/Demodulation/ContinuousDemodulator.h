#pragma once

#include "../ModulationPoints.h"
#include "EqaliserAqua.h"
#include "TimePeakerDemod.h"
#include "aqua_defines.h"

#include <ipps.h>

#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace aq_demod
{
	// Continuous (streaming) CCM demodulator. SynchroniseIQ() runs the whole chain
	// on a block of oversampled IQ samples:
	//   Gardner timing recovery -> DC removal -> AGC -> Costas carrier loop with a
	//   decision-directed equaliser -> SNR estimate.
	// The loop and equaliser state is kept between calls.
	class ContinuousDemodulator
	{
	public:
		ContinuousDemodulator();
		~ContinuousDemodulator();

		bool Init(const char* modulation, int upsample_passed);
		void Reset();

		bool SynchroniseIQ(const std::vector<Ipp32fc>& passed_iq,
			std::vector<Ipp32fc>& synced_iq);

		void SetPllSpeed(double alpha);
		void SetTimingSpeed(double gain_mu);
		void SetEqualiserSpeed(double step);

		// Equaliser length (tap count). Applied in Init().
		void SetEqualiserTaps(int tap_count);
		int  GetEqualiserTaps() const { return eq_taps_; }

		// Equaliser algorithm: "MMA"/"CMA" (blind), "LMS" (decision-directed) or
		// "AUTO" (warm-up: blind MMA, then decision-directed LMS with hysteresis).
		// Any other value (including nullptr) selects AUTO.
		void SetEqualiserAlgorithm(const char* algorithm);

		// AUTO diagnostics: whether warm-up is active and whether the constellation is
		// locked.
		bool IsEqualiserWarmUp() const { return equal_.IsWarmUpActive(); }
		bool IsEqualiserLocked() const { return equal_.IsLocked(); }

		double GetPllSpeed() const { return current_pll_speed_; }
		double GetTimingSpeed() const { return current_timing_speed_; }
		double GetEqualiserSpeed() const { return current_eq_speed_; }

		aqua::Limits<double> GetPllSpeedLimits() const { return kPllSpeedLimits; }
		aqua::Limits<double> GetTimingSpeedLimits() const { return kTimingSpeedLimits; }
		aqua::Limits<double> GetEqualiserSpeedLimits() const { return kEqualiserSpeedLimits; }

		double GetPllSpeedDefault() const { return kPllSpeedDefault; }
		double GetTimingSpeedDefault() const { return GetTimingSpeedLimits().mid(); }
		double GetEqualiserSpeedDefault() const { return kEqualiserSpeedDefault; }
		int GetEqualiserTapsDefault() const { return kEqualiserTapsDefault; }

		double GetFreqOffset() const { return freq_offset_; }
		double GetLastSNR() const { return snr_db_; }
		// Current symbol period estimate over the nominal (1.0 means exactly nominal).
		double GetSymbolRateRatio() const { return ted_man_.GetTimeError(); }

	private:
		void UpdateLoopCoefficients();
		void ApplyAGC(std::vector<Ipp32fc>& signal);
		// Slow (exponentially smoothed) DC estimate. Subtracting a per-block mean on
		// short blocks produces a block artifact and noticeably hurts SNR, so the
		// estimate is smoothed sample by sample.
		void RemoveDc(std::vector<Ipp32fc>& signal);

		Ipp32fc CorrectPhase(const Ipp32fc& sample) const;
		Ipp32fc GetDecision(const Ipp32fc& sample, double& phase_error) const;
		void    UpdatePll(double phase_error);
		void    UpdateSNR(const Ipp32fc& corrected, const Ipp32fc& decision);

	private:
		GardnerTED   ted_man_;
		EqaliserAqua equal_;

		std::vector<Ipp32fc> pivots_;

		int upsample_passed_ = 1;
		double prev_agc_power_ = 1.;
		double target_power_ = 1.0;

		// AGC normalises the constellation amplitude for the slicer.
		double agc_power_ = 1.0;
		double agc_alpha_ = 0.001;

		// Costas-loop speed (GNU Radio uses ~2*pi/100 as default; the range is
		// typically 2*pi/200 .. 2*pi/100).
		double current_alpha_ = 0.005;
		double damping_ = 1.0 / std::sqrt(2.0);

		// Loop speeds in real parameter units.
		double current_pll_speed_ = 5e-4;
		double current_timing_speed_ = 0.03;
		double current_eq_speed_ = 2.55e-3;

		// Default equaliser length.
		int eq_taps_ = kEqualiserTapsDefault;

		// Second-order loop gains (alpha and beta in GNU Radio).
		double pll_phase_ = 0.0; // proportional gain (alpha)
		double pll_freq_ = 0.0;  // integral gain (beta)

		// Carrier phase and frequency, rad and rad/symbol.
		double phase_ = 0.0;
		double freq_offset_ = 0.0;

		// Loop bounds in real parameter units; legacy PLL default.
		static constexpr aqua::Limits<double> kPllSpeedLimits{ 1e-4, 5e-3 };
		static constexpr aqua::Limits<double> kTimingSpeedLimits{ GardnerTED::kGainMuMin, GardnerTED::kGainMuMax };
		static constexpr aqua::Limits<double> kEqualiserSpeedLimits{ EqaliserAqua::kBlindStepMin, EqaliserAqua::kBlindStepMax };
		static constexpr double kPllSpeedDefault{ 5e-4 };
		// Small equaliser speed: blind algorithms (MMA/CMA) have a residual gain error
		// on non-circular (QAM) constellations, so a large step hurts SNR. A small step
		// keeps the filter near the matched state.
		static constexpr double kEqualiserSpeedDefault{ 1e-4 };
		// 21 taps are enough to suppress ISI from the RRC shaping (7 taps left SNR at
		// ~28.5 dB instead of ~29.7 dB).
		static constexpr int kEqualiserTapsDefault{ 21 };

		// Normalized frequency range of the fine loop.
		static constexpr double max_freq_offset_ = 0.05;

		// Symbol-level DC estimate; window in symbols (~243 Hz at 25 MHz).
		Ipp32fc dc_estimate_{ 0.0f, 0.0f };
		static constexpr int kDcSmoothSymbols{ 1 << 14 };

		double signal_power_ = 0.0;
		double error_power_ = 0.0;
		double snr_db_ = 0.0;
	};

}
