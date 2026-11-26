#pragma once

#include <ipps.h>

#include <vector>

namespace aq_mod
{
	// Adds complex white Gaussian noise to IQ samples.
	//
	// The level is relative to the signal power: -30 dB means the noise power is
	// 30 dB below the signal power (SNR = 30 dB). A level at or below
	// kDisabledLevelDb (-1000 dB) means "no noise".
	class GaussianNoiseMaker
	{
	public:
		static constexpr double kDisabledLevelDb = -1000.0;

		GaussianNoiseMaker();
		~GaussianNoiseMaker();

		// Changes only the noise level; the generator state is preserved.
		void SetNoiseLevel(double db) { noise_level_db_ = db; }
		double GetNoiseLevel() const { return noise_level_db_; }

		void SetEnabled(bool enabled) { enabled_ = enabled; }
		bool IsEnabled() const { return enabled_; }

		// Re-initializes the generator state (the noise level is unchanged).
		void Reset();

		// Adds noise to the given IQ samples in place.
		void Process(std::vector<Ipp32fc>& data);

	private:
		void InitState();

	private:
		double noise_level_db_ = kDisabledLevelDb;
		bool   enabled_ = false;

		IppsRandGaussState_32f* state_ = nullptr;
		std::vector<Ipp32f> noise_buffer_;
	};
}
