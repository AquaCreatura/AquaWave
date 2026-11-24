#include "EqaliserAqua.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
	inline double Power(const Ipp32fc& x)
	{
		return double(x.re) * x.re + double(x.im) * x.im;
	}
}

void EqaliserAqua::UpdateTaps(const Ipp32fc& error, double mu)
{
	const int size = static_cast<int>(taps_.size());
	const Ipp32fc coeff = {
		static_cast<Ipp32f>(mu * error.re),
		static_cast<Ipp32f>(-mu * error.im)
	};

	ippsMulC_32fc(history_.data(), coeff, update_.data(), size);
	ippsAdd_32fc_I(update_.data(), taps_.data(), size);
}
void EqaliserAqua::UpdateCma()
{
	const double error = cma_modulus_ - Power(last_output_);
	const Ipp32fc gradient = {
		static_cast<Ipp32f>(error * last_output_.re),
		static_cast<Ipp32f>(error * last_output_.im)
	};

	UpdateTaps(gradient, blind_step_);
}

void EqaliserAqua::UpdateMma()
{
	const Ipp32fc gradient = {
		static_cast<Ipp32f>(last_output_.re * (mma_real_modulus_ - last_output_.re * last_output_.re)),
		static_cast<Ipp32f>(last_output_.im * (mma_imag_modulus_ - last_output_.im * last_output_.im))
	};

	UpdateTaps(gradient, blind_step_);
}

void EqaliserAqua::UpdateLms(const Ipp32fc& pivot)
{
	const Ipp32fc error = { pivot.re - last_output_.re, pivot.im - last_output_.im };
	UpdateTaps(error, dd_step_);
}

void EqaliserAqua::UpdateNlms(const Ipp32fc& pivot)
{
	const Ipp32fc error = { pivot.re - last_output_.re, pivot.im - last_output_.im };

	double input_power = 0.0;
	for (const Ipp32fc& sample : history_)
		input_power += Power(sample);

	UpdateTaps(error, dd_nlms_step_ / (input_power + 1e-8));
}

void EqaliserAqua::UpdateBlind()
{
	switch (blind_algorithm_)
	{
	case BlindAlgorithm::CMA:
		UpdateCma();
		break;

	case BlindAlgorithm::MMA:
		UpdateMma();
		break;
	}
}

void EqaliserAqua::UpdateDd(const Ipp32fc& pivot)
{
	if (Power(last_output_) < 1e-12)
		return;

	switch (dd_algorithm_)
	{
	case DdAlgorithm::LMS:
		UpdateLms(pivot);
		break;

	case DdAlgorithm::NLMS:
		UpdateNlms(pivot);
		break;
	}
}

void EqaliserAqua::UpdateLockMetric(const Ipp32fc& pivot)
{
	// Ошибка решения: расстояние от выхода эквалайзера до ближайшей точки
	// созвездия. Сглаживаем по отсчётам, чтобы гистерезис не «дёргался».
	const Ipp32fc error = { pivot.re - last_output_.re, pivot.im - last_output_.im };
	const double error_power = Power(error);
	lock_metric_ += lock_alpha_ * (error_power - lock_metric_);
}

// ---------- Реализация класса ----------

EqaliserAqua::EqaliserAqua(int tap_count)
	: taps_(std::max(1, tap_count)),
	history_(taps_.size()),
	update_(taps_.size())
{
	Reset();
}

void EqaliserAqua::SetTapCount(int tap_count)
{
	tap_count = std::max(1, tap_count);
	if (static_cast<int>(taps_.size()) == tap_count)
		return;

	taps_.assign(tap_count, Ipp32fc{ 0.0f, 0.0f });
	history_.assign(tap_count, Ipp32fc{ 0.0f, 0.0f });
	update_.assign(tap_count, Ipp32fc{ 0.0f, 0.0f });
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

	// Warm-up всегда начинается заново со слепого алгоритма.
	warmup_dd_ = false;
	lock_metric_ = 1.0;
}

bool EqaliserAqua::IsValid() const
{
	for (const Ipp32fc& tap : taps_)
	{
		if (!std::isfinite(tap.re) || !std::isfinite(tap.im))
			return false;
	}

	return std::isfinite(last_output_.re) && std::isfinite(last_output_.im);
}

void EqaliserAqua::Process(Ipp32fc& sample)
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
	sample = output;
}

// ---- Обновление ----

void EqaliserAqua::Update(const Ipp32fc& pivot)
{
	if (auto_warmup_)
	{
		// Режим AUTO: следим за захватом и переключаемся слепой <-> решающий
		// с гистерезисом (lock_on_ < lock_off_), т.е. «вперёд-назад».
		UpdateLockMetric(pivot);

		if (!warmup_dd_ && lock_metric_ < lock_on_)
			warmup_dd_ = true;
		else if (warmup_dd_ && lock_metric_ > lock_off_)
			warmup_dd_ = false;

		if (warmup_dd_)
			UpdateDd(pivot);
		else
			UpdateBlind();
		return;
	}

	if (blind_direct_decision_)
		UpdateDd(pivot);
	else
		UpdateBlind();
		
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

	// Минимальное расстояние между точками созвездия: по нему масштабируем
	// пороги захвата. Плотные созвездия имеют малое расстояние, поэтому порог
	// становится строже и warm-up не переходит на LMS раньше времени.
	double min_dist_sq = std::numeric_limits<double>::max();
	for (size_t i = 0; i < pivots.size(); ++i)
	{
		for (size_t j = i + 1; j < pivots.size(); ++j)
		{
			const Ipp32fc delta = { pivots[i].re - pivots[j].re, pivots[i].im - pivots[j].im };
			min_dist_sq = std::min(min_dist_sq, Power(delta));
		}
	}

	if (min_dist_sq < std::numeric_limits<double>::max())
	{
		// Порог захвата масштабируется минимальным расстоянием созвездия, а
		// допустимая ошибка решения (в долях d_min^2) тем меньше, чем плотнее
		// созвездие: на плотных QAM решающий алгоритм склонен к размножению
		// ошибок, поэтому его включают только при хорошо раскрытом глазе.
		const double bits = std::log2(static_cast<double>(pivots.size()));
		const double lock_ratio = (bits <= 8.0) ? 0.30 : 0.0625;
		// Гистерезис: возврат на слепой алгоритм при ошибке вчетверо выше.
		lock_on_ = min_dist_sq * lock_ratio;
		lock_off_ = lock_on_ * 4.0;
	}
}

// ---- Настройка параметров ----

void EqaliserAqua::EnableDirectDecision(bool enabled)
{
	blind_direct_decision_ = enabled;
}

void EqaliserAqua::SetAutoWarmUp(bool enabled)
{
	auto_warmup_ = enabled;
	// Включение AUTO всегда перезапускает warm-up со слепого алгоритма.
	warmup_dd_ = false;
	lock_metric_ = 1.0;
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
