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

	explicit EqaliserAqua(int tap_count = 11);

	void Reset();

	// Обрабатывает входной отсчёт (с коррекцией фазы),
	// возвращает выровненный сигнал и сохраняет историю для обновления.
	Ipp32fc Process(const Ipp32fc& sample);

	// Обновление коэффициентов по решению (символу созвездия).
	// Вызывается после получения решения по выходу эквалайзера.
	void Update(const Ipp32fc& pivot);

	// Инициализация параметров слепых алгоритмов по точкам созвездия.
	void InitFromPivots(const std::vector<Ipp32fc>& pivots);

	void EnableBlind(bool enabled);
	void SetBlindAlgorithm(BlindAlgorithm algorithm);
	void SetDdAlgorithm(DdAlgorithm algorithm);
	void SetBlindStep(double step); // 1e-4 ... 5e-3
	void SetDdStep(double step);    // 0.001 ... 0.01 (для NLMS используется отдельный коэффициент)

private:
	void UpdateBlind();
	void UpdateDd(const Ipp32fc& pivot);   // больше не принимает corrected

	std::vector<Ipp32fc> taps_;
	std::vector<Ipp32fc> history_;
	std::vector<Ipp32fc> update_;

	Ipp32fc last_output_{ 0.0f, 0.0f };

	BlindAlgorithm blind_algorithm_ = BlindAlgorithm::MMA;
	DdAlgorithm    dd_algorithm_ = DdAlgorithm::NLMS;

	double blind_step_ = 0.0001;
	double dd_step_ = 0.005;
	double dd_nlms_step_ = 0.5;   // используется только для NLMS

	double cma_modulus_ = 1.0;
	double mma_real_modulus_ = 1.0;
	double mma_imag_modulus_ = 1.0;

	bool blind_enabled_ = true;
};