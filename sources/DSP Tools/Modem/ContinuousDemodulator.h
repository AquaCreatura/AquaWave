#pragma once

#include "EqaliserAqua.h"
#include "ModulationPoints.h"
#include "TimePeakerDemod.h"
#include "aqua_defines.h"

#include <ipps.h>

#include <cmath>
#include <vector>

#ifndef  MPI
	#define M_PI 3.14159265358979323846
#endif // ! MPI



namespace aq_demod
{

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

		double GetPllSpeed() const { return current_pll_speed_; }
		double GetTimingSpeed() const { return current_timing_speed_; }
		double GetEqualiserSpeed() const { return current_eq_speed_; }

		aqua::Limits<double> GetPllSpeedLimits() const { return kPllSpeedLimits; }
		aqua::Limits<double> GetTimingSpeedLimits() const { return kTimingSpeedLimits; }
		aqua::Limits<double> GetEqualiserSpeedLimits() const { return kEqualiserSpeedLimits; }

		double GetPllSpeedDefault() const { return kPllSpeedDefault; }
		double GetTimingSpeedDefault() const { return GetTimingSpeedLimits().mid(); }
		double GetEqualiserSpeedDefault() const { return GetEqualiserSpeedLimits().mid(); }

		double GetFreqOffset() const { return freq_offset_; }
		double GetLastSNR() const { return snr_db_; }
		// Отношение текущей оценки периода символа к номинальной (1.0 — точно в номинале).
		double GetSymbolRateRatio() const { return ted_man_.GetTimeError(); }

	private:
		void UpdateLoopCoefficients();
		void ApplyAGC(std::vector<Ipp32fc>& signal);

		Ipp32fc CorrectPhase(const Ipp32fc& sample) const;
		Ipp32fc GetDecision(const Ipp32fc& sample, double& phase_error) const;
		void    UpdatePll(const double & error);
		void    UpdateSNR(const Ipp32fc sample, const Ipp32fc decission);
	private:

		GardnerTED ted_man_;
		EqaliserAqua equal_;


		std::vector<Ipp32fc> pivots_;

		int upsample_passed_ = 1;
		double prev_agc_power_ = 1.;
		double target_power_ = 1.0;

		// AGC normalizes the constellation amplitude for the slicer.
		double agc_power_  = 1.0;
		double agc_alpha_  = 0.001;

		// GNU Radio uses ~2*pi/100 as the default Costas-loop bandwidth.
		// The range is typically between 2*pi/200 and 2*pi/100.
		double current_alpha_ = 0.005; // Значение по умолчанию для alpha
		double damping_ = 1.0 / std::sqrt(2.0);

		// Готовые значения скоростей для окна (в единицах реального параметра).
		double current_pll_speed_ = 5e-4;
		double current_timing_speed_ = 0.03;
		double current_eq_speed_ = 2.55e-3;

		// Gains for the second-order loop (alpha and beta in GNU Radio).
		double pll_phase_ = 0.0; // Proportional gain (alpha)
		double pll_freq_ = 0.0;  // Integral gain (beta)

								 // Carrier phase and frequency, rad and rad/symbol.
		double phase_ = 0.0;
		double freq_offset_ = 0.0;

		// Границы петель в единицах реальных параметров; legacy-дефолт PLL.
		static constexpr aqua::Limits<double> kPllSpeedLimits{ 1e-4, 5e-3 };
		static constexpr aqua::Limits<double> kTimingSpeedLimits{ GardnerTED::kGainMuMin, GardnerTED::kGainMuMax };
		static constexpr aqua::Limits<double> kEqualiserSpeedLimits{ EqaliserAqua::kBlindStepMin, EqaliserAqua::kBlindStepMax };
		static constexpr double kPllSpeedDefault{ 5e-4 };

		// Normalized frequency range of the fine loop.
		// Standard values from GNU Radio examples.
		static constexpr double max_freq_offset_ = 0.05;

		double signal_power_ = 0.0;
		double error_power_ = 0.0;
		double snr_db_ = 0.0;
	};

}
