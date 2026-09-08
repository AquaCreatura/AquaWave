#include "EqaliserAqua.h"

#include <algorithm>
#include <cmath>

namespace
{
	inline double Power(const Ipp32fc& x)
	{
		return double(x.re) * x.re + double(x.im) * x.im;
	}

	// Вспомогательная функция для обновления весов
	void UpdateTaps(std::vector<Ipp32fc>& taps, const std::vector<Ipp32fc>& x,
		const Ipp32fc& error, double mu, std::vector<Ipp32fc>& update)
	{
		const int size = static_cast<int>(taps.size());
		const Ipp32fc coeff = { static_cast<Ipp32f>(mu * error.re),
			static_cast<Ipp32f>(-mu * error.im) };

		ippsMulC_32fc(x.data(), coeff, update.data(), size);
		ippsAdd_32fc_I(update.data(), taps.data(), size);
	}

	// ----- Слепые алгоритмы -----
	void UpdateCma(std::vector<Ipp32fc>& taps, const std::vector<Ipp32fc>& x,
		const Ipp32fc& y, double mu, double modulus,
		std::vector<Ipp32fc>& update)
	{
		const double error = modulus - Power(y);
		const Ipp32fc gradient = {
			static_cast<Ipp32f>(error * y.re),
			static_cast<Ipp32f>(error * y.im)
		};
		UpdateTaps(taps, x, gradient, mu, update);
	}

	void UpdateMma(std::vector<Ipp32fc>& taps, const std::vector<Ipp32fc>& x,
		const Ipp32fc& y, double mu, double real_modulus,
		double imag_modulus, std::vector<Ipp32fc>& update)
	{
		const Ipp32fc gradient = {
			static_cast<Ipp32f>(y.re * (real_modulus - y.re * y.re)),
			static_cast<Ipp32f>(y.im * (imag_modulus - y.im * y.im))
		};
		UpdateTaps(taps, x, gradient, mu, update);
	}

	// ----- Режим с решением (Decision-Directed) -----
	void UpdateLms(std::vector<Ipp32fc>& taps, const std::vector<Ipp32fc>& x,
		const Ipp32fc& y, const Ipp32fc& pivot,
		double mu, std::vector<Ipp32fc>& update)
	{
		const Ipp32fc error = { pivot.re - y.re, pivot.im - y.im };
		UpdateTaps(taps, x, error, mu, update);
	}

	void UpdateNlms(std::vector<Ipp32fc>& taps, const std::vector<Ipp32fc>& x,
		const Ipp32fc& y, const Ipp32fc& pivot,
		double mu, std::vector<Ipp32fc>& update)
	{
		const Ipp32fc error = { pivot.re - y.re, pivot.im - y.im };

		double input_power = 0.0;
		for (const Ipp32fc& sample : x)
			input_power += Power(sample);

		// Нормированный шаг: mu / (мощность + эпсилон)
		UpdateTaps(taps, x, error, mu / (input_power + 1e-8), update);
	}
}

// ---------- Реализация класса ----------

EqaliserAqua::EqaliserAqua(int tap_count)
	: taps_(std::max(1, tap_count)),
	history_(taps_.size()),
	update_(taps_.size())
{
	Reset();
}

void EqaliserAqua::Reset()
{
	std::fill(taps_.begin(), taps_.end(), Ipp32fc{ 0.0f, 0.0f });
	std::fill(history_.begin(), history_.end(), Ipp32fc{ 0.0f, 0.0f });
	std::fill(update_.begin(), update_.end(), Ipp32fc{ 0.0f, 0.0f });

	// Центральный отвод = 1 (фильтр пропускает сигнал без искажений)
	taps_[taps_.size() / 2] = { 1.0f, 0.0f };
	last_output_ = { 0.0f, 0.0f };
}

Ipp32fc EqaliserAqua::Process(const Ipp32fc& sample)
{
	// Сдвиг истории (новый отсчёт на первое место)
	for (size_t i = history_.size() - 1; i > 0; --i)
		history_[i] = history_[i - 1];
	history_[0] = sample;

	// Вычисление выходного сигнала (фильтрация)
	Ipp32fc output = { 0.0f, 0.0f };
	for (size_t i = 0; i < taps_.size(); ++i)
	{
		output.re += taps_[i].re * history_[i].re + taps_[i].im * history_[i].im;
		output.im += taps_[i].re * history_[i].im - taps_[i].im * history_[i].re;
	}

	last_output_ = output;
	return output;
}

// ---- Обновление ----

void EqaliserAqua::Update(const Ipp32fc& pivot)
{
	if (blind_enabled_)
		UpdateBlind();
	else
		UpdateDd(pivot);
}

void EqaliserAqua::UpdateBlind()
{
	switch (blind_algorithm_)
	{
	case BlindAlgorithm::CMA:
		UpdateCma(taps_, history_, last_output_, blind_step_,
			cma_modulus_, update_);
		break;
	case BlindAlgorithm::MMA:
		UpdateMma(taps_, history_, last_output_, blind_step_,
			mma_real_modulus_, mma_imag_modulus_, update_);
		break;
	}
}

void EqaliserAqua::UpdateDd(const Ipp32fc& pivot)
{
	// Защита от слишком маленького выходного сигнала (избегаем деления на ноль в NLMS)
	if (Power(last_output_) < 1e-12)
		return;

	// Ошибка = решение – выход эквалайзера (без поворота фазы!)
	switch (dd_algorithm_)
	{
	case DdAlgorithm::LMS:
		UpdateLms(taps_, history_, last_output_, pivot, dd_step_, update_);
		break;
	case DdAlgorithm::NLMS:
		// Используем отдельный коэффициент для NLMS
		UpdateNlms(taps_, history_, last_output_, pivot, dd_nlms_step_, update_);
		break;
	}
}

// ---- Инициализация параметров по созвездию ----

void EqaliserAqua::InitFromPivots(const std::vector<Ipp32fc>& pivots)
{
	if (pivots.empty())
		return;

	const int size = static_cast<int>(pivots.size());

	std::vector<Ipp32f> power(size);
	std::vector<Ipp32f> power_sq(size);
	std::vector<Ipp32f> real(size);
	std::vector<Ipp32f> imag(size);
	std::vector<Ipp32f> real_sq(size);
	std::vector<Ipp32f> imag_sq(size);
	std::vector<Ipp32f> real_4(size);
	std::vector<Ipp32f> imag_4(size);

	// Мощность и её квадрат для CMA
	ippsPowerSpectr_32fc(pivots.data(), power.data(), size);
	ippsMul_32f(power.data(), power.data(), power_sq.data(), size);

	Ipp32f mean_power = 0.0f;
	Ipp32f mean_power_sq = 0.0f;
	ippsMean_32f(power.data(), size, &mean_power, ippAlgHintFast);
	ippsMean_32f(power_sq.data(), size, &mean_power_sq, ippAlgHintFast);

	if (mean_power > 1e-12f)
		cma_modulus_ = double(mean_power_sq) / mean_power;

	// Действительная и мнимая части для MMA
	ippsReal_32fc(pivots.data(), real.data(), size);
	ippsImag_32fc(pivots.data(), imag.data(), size);

	ippsMul_32f(real.data(), real.data(), real_sq.data(), size);
	ippsMul_32f(imag.data(), imag.data(), imag_sq.data(), size);

	ippsMul_32f(real_sq.data(), real_sq.data(), real_4.data(), size);
	ippsMul_32f(imag_sq.data(), imag_sq.data(), imag_4.data(), size);

	Ipp32f mean_real_sq = 0.0f, mean_imag_sq = 0.0f;
	Ipp32f mean_real_4 = 0.0f, mean_imag_4 = 0.0f;
	ippsMean_32f(real_sq.data(), size, &mean_real_sq, ippAlgHintFast);
	ippsMean_32f(imag_sq.data(), size, &mean_imag_sq, ippAlgHintFast);
	ippsMean_32f(real_4.data(), size, &mean_real_4, ippAlgHintFast);
	ippsMean_32f(imag_4.data(), size, &mean_imag_4, ippAlgHintFast);

	if (mean_real_sq > 1e-12f)
		mma_real_modulus_ = double(mean_real_4) / mean_real_sq;
	if (mean_imag_sq > 1e-12f)
		mma_imag_modulus_ = double(mean_imag_4) / mean_imag_sq;
}

// ---- Настройка параметров ----

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
	blind_step_ = std::clamp(step, kBlindStepMin, kBlindStepMax);
}

void EqaliserAqua::SetDdStep(double step)
{
	dd_step_ = std::clamp(step, kDdStepMin, kDdStepMax);
	// Для NLMS используем отдельный коэффициент (можно изменить по необходимости)
	dd_nlms_step_ = std::clamp(step * taps_.size(), 0.1, 1.0);
}