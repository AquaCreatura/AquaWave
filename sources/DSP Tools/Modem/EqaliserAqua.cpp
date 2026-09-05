
#include "EqaliserAqua.h"

#include <algorithm>
#include <cmath>

namespace
{
	inline Ipp32fc Mul(const Ipp32fc& a, const Ipp32fc& b)
	{
		return { a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re };
	}

	inline Ipp32fc Conj(const Ipp32fc& x)
	{
		return { x.re, -x.im };
	}

	inline Ipp32fc Rotate(const Ipp32fc& x, double phase)
	{
		const float c = static_cast<float>(std::cos(phase));
		const float s = static_cast<float>(std::sin(phase));
		return { x.re * c - x.im * s, x.re * s + x.im * c };
	}

	inline double Power(const Ipp32fc& x)
	{
		return double(x.re) * x.re + double(x.im) * x.im;
	}

	void UpdateCma(std::vector<Ipp32fc>& taps, const std::vector<Ipp32fc>& x, const Ipp32fc& y, double mu, double modulus)
	{
		const double error = modulus - Power(y);
		const Ipp32fc gradient = { static_cast<Ipp32f>(error * y.re), static_cast<Ipp32f>(error * y.im) };

		for (size_t i = 0; i < taps.size(); ++i) {
			const Ipp32fc update = Mul(x[i], Conj(gradient));
			taps[i].re += static_cast<Ipp32f>(mu * update.re);
			taps[i].im += static_cast<Ipp32f>(mu * update.im);
		}
	}

	void UpdateMma(std::vector<Ipp32fc>& taps, const std::vector<Ipp32fc>& x, const Ipp32fc& y, double mu, double real_modulus, double imag_modulus)
	{
		const Ipp32fc gradient = {
			static_cast<Ipp32f>(y.re * (real_modulus - y.re * y.re)),
			static_cast<Ipp32f>(y.im * (imag_modulus - y.im * y.im))
		};

		for (size_t i = 0; i < taps.size(); ++i) {
			const Ipp32fc update = Mul(x[i], Conj(gradient));
			taps[i].re += static_cast<Ipp32f>(mu * update.re);
			taps[i].im += static_cast<Ipp32f>(mu * update.im);
		}
	}

	void UpdateLms(std::vector<Ipp32fc>& taps, const std::vector<Ipp32fc>& x, const Ipp32fc& y, const Ipp32fc& pivot, double mu)
	{
		const Ipp32fc error = { pivot.re - y.re, pivot.im - y.im };

		for (size_t i = 0; i < taps.size(); ++i) {
			const Ipp32fc update = Mul(x[i], Conj(error));
			taps[i].re += static_cast<Ipp32f>(mu * update.re);
			taps[i].im += static_cast<Ipp32f>(mu * update.im);
		}
	}

	void UpdateNlms(std::vector<Ipp32fc>& taps, const std::vector<Ipp32fc>& x, const Ipp32fc& y, const Ipp32fc& pivot, double mu)
	{
		const Ipp32fc error = { pivot.re - y.re, pivot.im - y.im };

		double input_power = 0.0;
		for (const Ipp32fc& sample : x)
			input_power += Power(sample);

		const double gain = mu / (input_power + 1e-8);

		for (size_t i = 0; i < taps.size(); ++i) {
			const Ipp32fc update = Mul(x[i], Conj(error));
			taps[i].re += static_cast<Ipp32f>(gain * update.re);
			taps[i].im += static_cast<Ipp32f>(gain * update.im);
		}
	}
}

EqaliserAqua::EqaliserAqua(int tap_count)
	: taps_(std::max(1, tap_count)), history_(taps_.size())
{
	Reset();
}

void EqaliserAqua::Reset()
{
	std::fill(taps_.begin(), taps_.end(), Ipp32fc{ 0.0f, 0.0f });
	std::fill(history_.begin(), history_.end(), Ipp32fc{ 0.0f, 0.0f });

	taps_[taps_.size() / 2] = { 1.0f, 0.0f };
	last_output_ = { 0.0f, 0.0f };
}

Ipp32fc EqaliserAqua::Process(const Ipp32fc& sample)
{
	for (size_t i = history_.size() - 1; i > 0; --i)
		history_[i] = history_[i - 1];

	history_[0] = sample;

	Ipp32fc output = { 0.0f, 0.0f };

	for (size_t i = 0; i < taps_.size(); ++i) {
		const Ipp32fc term = Mul(Conj(taps_[i]), history_[i]);
		output.re += term.re;
		output.im += term.im;
	}

	last_output_ = output;
	return output;
}

void EqaliserAqua::Update(const Ipp32fc& corrected, const Ipp32fc& pivot)
{
	if (blind_enabled_)
		UpdateBlind();
	else
		UpdateDd(corrected, pivot);
}

void EqaliserAqua::UpdateBlind()
{
	switch (blind_algorithm_) {
	case BlindAlgorithm::CMA:
		UpdateCma(taps_, history_, last_output_, blind_step_, cma_modulus_);
		break;

	case BlindAlgorithm::MMA:
		UpdateMma(taps_, history_, last_output_, blind_step_, mma_real_modulus_, mma_imag_modulus_);
		break;
	}
}

void EqaliserAqua::UpdateDd(const Ipp32fc& corrected, const Ipp32fc& pivot)
{
	if (Power(last_output_) < 1e-12)
		return;

	// corrected = last_output * exp(j * phase)
	// ¬озвращаем pivot в систему координат самого equaliser.
	const double phase = std::atan2(
		corrected.im * last_output_.re - corrected.re * last_output_.im,
		corrected.re * last_output_.re + corrected.im * last_output_.im);

	const Ipp32fc pivot_eq = Rotate(pivot, -phase);

	switch (dd_algorithm_) {
	case DdAlgorithm::LMS:
		UpdateLms(taps_, history_, last_output_, pivot_eq, dd_step_);
		break;

	case DdAlgorithm::NLMS:
		UpdateNlms(taps_, history_, last_output_, pivot_eq, dd_step_);
		break;
	}
}

void EqaliserAqua::EnableBlind(bool enabled)
{
	blind_enabled_ = enabled;
}

void EqaliserAqua::SetBlindAlgorithm(BlindAlgorithm algorithm)
{
	blind_algorithm_ = algorithm;
}

void EqaliserAqua::SetDdAlgorithm(DdAlgorithm algorithm)
{
	dd_algorithm_ = algorithm;
}

void EqaliserAqua::SetBlindStep(double step)
{
	blind_step_ = step;
}

void EqaliserAqua::SetDdStep(double step)
{
	dd_step_ = step;
}

void EqaliserAqua::SetCmaModulus(double modulus)
{
	cma_modulus_ = modulus;
}

void EqaliserAqua::SetMmaModulus(double real_modulus, double imag_modulus)
{
	mma_real_modulus_ = real_modulus;
	mma_imag_modulus_ = imag_modulus;
}

