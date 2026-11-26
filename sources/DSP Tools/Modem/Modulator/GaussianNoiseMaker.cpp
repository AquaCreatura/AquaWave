#include "GaussianNoiseMaker.h"

#include <cmath>

namespace
{
	// Fixed seed so a run is reproducible.
	constexpr unsigned int kSeed = 0x5eed1234u;
}

namespace aq_mod
{

	GaussianNoiseMaker::GaussianNoiseMaker()
	{
		InitState();
	}

	GaussianNoiseMaker::~GaussianNoiseMaker()
	{
		if (state_)
		{
			ippsFree(state_);
			state_ = nullptr;
		}
	}

	void GaussianNoiseMaker::InitState()
	{
		if (!state_)
		{
			int state_size = 0;
			if (ippsRandGaussGetSize_32f(&state_size) != ippStsNoErr)
				return;
			state_ = reinterpret_cast<IppsRandGaussState_32f*>(ippsMalloc_8u(state_size));
		}

		// Standard normal (mean 0, stddev 1); Process() scales it to the required
		// noise power, so SetNoiseLevel() need not touch the state.
		if (state_)
			ippsRandGaussInit_32f(state_, 0.0f, 1.0f, kSeed);
	}

	void GaussianNoiseMaker::Reset()
	{
		InitState();
	}

	void GaussianNoiseMaker::Process(std::vector<Ipp32fc>& data)
	{
		if (!enabled_ || noise_level_db_ <= kDisabledLevelDb || data.empty() || !state_)
			return;

		// Average signal power over the block.
		double signal_power = 0.0;
		for (const Ipp32fc& sample : data)
			signal_power += static_cast<double>(sample.re) * sample.re + static_cast<double>(sample.im) * sample.im;
		signal_power /= static_cast<double>(data.size());

		if (signal_power <= 0.0)
			return;

		const double noise_power = signal_power * std::pow(10.0, noise_level_db_ / 10.0);
		// The complex noise power is split equally between the I and Q components.
		const double sigma = std::sqrt(noise_power / 2.0);

		const int float_count = static_cast<int>(data.size() * 2);
		noise_buffer_.resize(float_count);
		if (ippsRandGauss_32f(noise_buffer_.data(), float_count, state_) != ippStsNoErr)
			return;

		// Ipp32fc is two contiguous Ipp32f values (re, im), so the complex vector can
		// be treated as an interleaved float vector of twice the length.
		Ipp32f* samples = reinterpret_cast<Ipp32f*>(data.data());
		ippsMulC_32f_I(static_cast<Ipp32f>(sigma), noise_buffer_.data(), float_count);
		ippsAdd_32f_I(noise_buffer_.data(), samples, float_count);
	}

}
