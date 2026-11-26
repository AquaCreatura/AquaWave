#pragma once

#include <ipps.h>

#include <vector>

namespace aq_demod
{
	// Gardner timing error detector with cubic interpolation. Recovers symbol timing
	// from an oversampled (2 or 4 samples/symbol) stream and decimates to one sample
	// per symbol. State is kept between Process() calls, so a stream can be processed
	// block by block.
	class GardnerTED
	{
	public:
		// Proportional gain (gain_mu) range of the timing loop. The integral gain
		// (gain_omega) is derived from it as mu^2 / 4.
		static constexpr double kGainMuMin = 0.01;
		static constexpr double kGainMuMax = 0.05;

		explicit GardnerTED(int upsample_koeff = 4);
		~GardnerTED();

		bool Process(const std::vector<Ipp32fc>& passed, std::vector<Ipp32fc>& out_samples);
		void Reset();
		void SetLoopGains(double gain_mu, double gain_omega);
		double GetTimeError() const;

	private:
		Ipp32fc Interpolate(const std::vector<Ipp32fc>& signal, double pos) const;
		Ipp32fc GetSample(const std::vector<Ipp32fc>& signal, int index) const;
		double GetError(const Ipp32fc& prev, const Ipp32fc& mid, const Ipp32fc& cur);
		void UpdateLoop(double error);
		void UpdateHistory(const std::vector<Ipp32fc>& signal);

	private:
		int upsample_koeff_;

		// Symbol period estimate in samples.
		double w_;

		// Position of the current symbol instant within the block.
		double pos_;

		// PI timing loop.
		double gain_mu_;
		double gain_omega_;

		// Bounds on the symbol period estimate.
		double omega_min_;
		double omega_max_;

		// Slow estimate of the average signal power.
		double power_;

		// Previous symbol-timed sample.
		Ipp32fc prev_symb_;
		bool have_prev_symb_;

		// Last few input samples, needed across block boundaries.
		std::vector<Ipp32fc> history_;
	};

} // namespace aq_demod
