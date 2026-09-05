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

	explicit EqaliserAqua(int tap_count = 11);

	void Reset();

	Ipp32fc Process(const Ipp32fc& sample);

	// corrected - sample после carrier correction.
	// pivot    - ближайшая точка созвездия.
	void Update(const Ipp32fc& corrected, const Ipp32fc& pivot);

	void EnableBlind(bool enabled);

	void SetBlindAlgorithm(BlindAlgorithm algorithm);
	void SetDdAlgorithm(DdAlgorithm algorithm);

	void SetBlindStep(double step);
	void SetDdStep(double step);

	// Для нормированных созвездий.
	void SetCmaModulus(double modulus);
	void SetMmaModulus(double real_modulus, double imag_modulus);

private:
	void UpdateBlind();
	void UpdateDd(const Ipp32fc& corrected, const Ipp32fc& pivot);

	std::vector<Ipp32fc> taps_;
	std::vector<Ipp32fc> history_;

	Ipp32fc last_output_{ 0.0f, 0.0f };

	BlindAlgorithm blind_algorithm_ = BlindAlgorithm::CMA;
	DdAlgorithm dd_algorithm_ = DdAlgorithm::NLMS;

	double blind_step_ = 0.001;
	double dd_step_ = 0.01;

	double cma_modulus_ = 1.0;
	double mma_real_modulus_ = 1.0;
	double mma_imag_modulus_ = 1.0;

	bool blind_enabled_ = true;
};

