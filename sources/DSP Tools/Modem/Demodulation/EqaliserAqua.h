#pragma once

#include <ipps.h>

#include <vector>

class EqaliserAqua
{
public:
	enum class BlindAlgorithm
	{
		CMA,
		MMA
	};

	enum class DdAlgorithm
	{
		LMS,
		NLMS
	};

	static constexpr double kBlindStepMin = 1e-4;
	static constexpr double kBlindStepMax = 5e-3;
	static constexpr double kDdStepMin = 1e-3;
	static constexpr double kDdStepMax = 1e-2;

	explicit EqaliserAqua(int tap_count = 7);

	// Длина фильтра (число отводов). Меняет размер истории и сбрасывает состояние.
	void SetTapCount(int tap_count);
	int  GetTapCount() const { return static_cast<int>(taps_.size()); }

	void Reset();
	bool IsValid() const;
	// Обрабатывает входной отсчёт (с коррекцией фазы),
	// возвращает выровненный сигнал и сохраняет историю для обновления.
	void Process(Ipp32fc& sample);

	// Обновление коэффициентов по решению (символу созвездия).
	// Вызывается после получения решения по выходу эквалайзера.
	void Update(const Ipp32fc& pivot);

	// Инициализация параметров слепых алгоритмов по точкам созвездия.
	void InitFromPivots(const std::vector<Ipp32fc>& pivots);

	void EnableDirectDecision(bool enabled);
	void SetBlindAlgorithm(BlindAlgorithm algorithm);
	void SetDdAlgorithm(DdAlgorithm algorithm);
	void SetBlindStep(double step); // 1e-4 ... 5e-3
	void SetDdStep(double step);    // 0.001 ... 0.01 (для NLMS используется отдельный коэффициент)

	// Warm-up (режим AUTO): сначала работает слепой алгоритм (MMA), а после
	// захвата созвездия эквалайзер переходит на решающий (LMS). Переключение
	// идёт по сглаженной ошибке решения и имеет гистерезис, поэтому возможен
	// и обратный переход (LMS -> MMA), если созвездие снова «поехало».
	void SetAutoWarmUp(bool enabled);
	bool IsWarmUpActive() const { return auto_warmup_; }
	// Текущее состояние warm-up: true — работаем по решению (LMS), false — слепо (MMA).
	bool IsLocked() const { return warmup_dd_; }

private:
	void UpdateBlind();
	void UpdateDd(const Ipp32fc& pivot);  
	void UpdateLockMetric(const Ipp32fc& pivot);

	void UpdateCma();
	void UpdateMma();
	void UpdateLms(const Ipp32fc& pivot);
	void UpdateNlms(const Ipp32fc& pivot);
	void UpdateTaps(const Ipp32fc& error, double mu);

private:
	std::vector<Ipp32fc> taps_;
	std::vector<Ipp32fc> history_;
	std::vector<Ipp32fc> update_;

	Ipp32fc last_output_{ 0.0f, 0.0f };

	BlindAlgorithm blind_algorithm_ = BlindAlgorithm::MMA;
	DdAlgorithm    dd_algorithm_ = DdAlgorithm::NLMS;

	double blind_step_ = 0.01;
	double dd_step_ = 0.01;
	double dd_nlms_step_ = 0.8;   // используется только для NLMS

	double cma_modulus_ = 1.0;
	double mma_real_modulus_ = 1.0;
	double mma_imag_modulus_ = 1.0;

	bool blind_direct_decision_ = true;

	// Warm-up (AUTO): слепой старт + переход на решающий алгоритм по захвату.
	bool   auto_warmup_ = false;
	bool   warmup_dd_ = false;
	// Сглаженная (EMA) ошибка решения; в начале намеренно завышена, чтобы
	// стартовать именно со слепого алгоритма.
	double lock_metric_ = 1.0;
	double lock_alpha_ = 1.0 / 1024.0;
	// Пороги гистерезиса, вычисляются из минимального расстояния созвездия:
	//   lock_on_  — ошибка ниже этого уровня => переходим на LMS;
	//   lock_off_ — ошибка выше этого уровня => возвращаемся на MMA.
	double lock_on_ = 0.0;
	double lock_off_ = 0.0;
};
