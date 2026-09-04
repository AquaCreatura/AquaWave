#pragma once

#include <ipps.h>
#include <vector>
#include <cmath>

#include "TimePeakerDemod.h"
#include "ModulationPoints.h"

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

		void SetAGCEnabled(bool enabled) { agc_enabled_ = enabled; }
		void SetPllSpeed(double speed);

		double GetFreqOffset() const { return freq_offset_; }
		double GetLastSNR() const { return snr_db_; }

	private:
		void UpdateLoopCoefficients();
		void ApplyAGC(std::vector<Ipp32fc>& signal);

		Ipp32fc CorrectPhase(const Ipp32fc& sample) const;
		Ipp32fc GetDecision(const Ipp32fc& sample, double& phase_error) const;

	private:
		GardnerTED ted_man_;

		std::vector<Ipp32fc> pivots_;

		int upsample_passed_ = 1;

		double target_power_ = 1.0;

		// AGC normalizes the constellation amplitude for the slicer.
		bool agc_enabled_ = true;
		double agc_power_ = 1.0;
		double agc_alpha_ = 0.01;

		// GNU Radio uses ~2*pi/100 as the default Costas-loop bandwidth.
		// The range is typically between 2*pi/200 and 2*pi/100.
		double current_alpha_ = 0.005; // Значение по умолчанию для alpha
		double damping_ = 1.0 / std::sqrt(2.0);

		// Gains for the second-order loop (alpha and beta in GNU Radio).
		double pll_phase_ = 0.0; // Proportional gain (alpha)
		double pll_freq_ = 0.0;  // Integral gain (beta)

								 // Carrier phase and frequency, rad and rad/symbol.
		double phase_ = 0.0;
		double freq_offset_ = 0.0;

		// Normalized frequency range of the fine loop.
		// Standard values from GNU Radio examples.
		static constexpr double max_freq_offset_ = 0.05;

		double signal_power_ = 0.0;
		double error_power_ = 0.0;
		double snr_db_ = 0.0;
	};

}