// FarrowInterpolator.cpp
#include "FarrowInterpolator.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>

FarrowInterpolator::FarrowInterpolator() : ratio_(1.0), virtual_index_(0.0) {
	tail_.fill({ 0.0f, 0.0f });
}
//Перед резамплингом отрабатывает КИХ фильтр
void FarrowInterpolator::SetResampleRatio(double resample_ratio) {
	if (resample_ratio <= 0.0 || resample_ratio > 200.0) {
		throw std::invalid_argument("Invalid resample ratio");
	}
	ratio_ = resample_ratio; //По факту резамплер для уменьшения ЧД, в иной концепции не предполагается
}

bool FarrowInterpolator::can_interpolate(int base_index, size_t input_size) const {
	// Нужны 3 точки: base-1, base, base+1, base+2
	// base, base+1 и base+2 должны быть доступны (в текущем блоке или через get_sample)

	if (base_index < -2 || base_index + 2 >= static_cast<int>(input_size)) {
		return false;
	}

	return true;
}

Ipp32fc FarrowInterpolator::get_sample(int index, const Ipp32fc* input, size_t input_size) const {
	if (index < 0) {
		// Берем из хвоста: индексы -1, -2, -3
		int tail_idx = 3 + index; // -1 -> 3, -2 -> 2, -3 -> 1
		if (tail_idx >= 0) {
			return tail_[tail_idx];
		}
		return { 0.0f, 0.0f };
	}

	if (static_cast<size_t>(index) < input_size) {
		return input[index];
	}

	return { 0.0f, 0.0f };
}

void FarrowInterpolator::update_tail(const Ipp32fc* input, size_t input_size) {
	// Всегда сохраняем последние 3 семпла текущего блока
	if (input_size < 3) return;

	size_t copy_count = 3;
	for (size_t i = 0; i < copy_count; ++i) {
		tail_[i] = input[input_size - copy_count + i];
	}
}



void FarrowInterpolator::process(
	const Ipp32fc* input,
	const size_t input_size,
	std::vector<Ipp32fc>& output)
{
	output.clear();
	if (input_size <= 4) return; //Работаем только с нормальными длинами

	// virtual_index_ указывает на позицию интерполяции
	// base = floor(virtual_index_) соответствует индексу y0
	// интерполяция выполняется на интервале [base, base+1]

	output.reserve(static_cast<size_t>(input_size / ratio_) + 10);
	while (true) {
		int base = static_cast<int>(floor(virtual_index_));
		float t = static_cast<float>(virtual_index_ - base);

		if (!can_interpolate(base, input_size)) {
			break;
		}

		Ipp32fc points[4];
		points[0] = get_sample(base - 1, input, input_size);
		points[1] = get_sample(base, input, input_size);
		points[2] = get_sample(base + 1, input, input_size);
		points[3] = get_sample(base + 2, input, input_size);

		FarrowCoeffs coeffs(points);
		output.push_back(coeffs.interpolate(t));

		virtual_index_ += ratio_;
	}
	// Обновляем хвост для следующего блока
	update_tail(input, input_size);
	virtual_index_ -= input_size;
}

void FarrowInterpolator::reset() {
	virtual_index_ = 0.0;
	tail_.fill({ 0.0f, 0.0f });
}

FarrowInterpolator::FarrowCoeffs::FarrowCoeffs(const Ipp32fc* samples) {
	// Входящие точки теперь:
	// samples[0] -> y_m1 (x = -1)
	// samples[1] -> y0   (x = 0)  <-- Начало интервала t
	// samples[2] -> y1   (x = 1)  <-- Конец интервала t
	// samples[3] -> y2   (x = 2)

	const Ipp32fc& y_m1 = samples[0];
	const Ipp32fc& y0 = samples[1];
	const Ipp32fc& y1 = samples[2];
	const Ipp32fc& y2 = samples[3];

	const float half = 0.5f;

	// Сплайн Catmull-Rom:
	// a0 = y0
	// a1 = 1/2 * (y1 - y_m1)
	// a2 = y_m1 - 5/2*y0 + 2*y1 - 1/2*y2
	// a3 = -1/2*y_m1 + 3/2*y0 - 3/2*y1 + 1/2*y2

	a0 = y0;

	a1.re = half * (y1.re - y_m1.re);
	a1.im = half * (y1.im - y_m1.im);

	a2.re = y_m1.re - 2.5f * y0.re + 2.0f * y1.re - 0.5f * y2.re;
	a2.im = y_m1.im - 2.5f * y0.im + 2.0f * y1.im - 0.5f * y2.im;

	a3.re = half * (-y_m1.re + 3.0f * y0.re - 3.0f * y1.re + y2.re);
	a3.im = half * (-y_m1.im + 3.0f * y0.im - 3.0f * y1.im + y2.im);
}

Ipp32fc FarrowInterpolator::FarrowCoeffs::interpolate(float t) const {
	Ipp32fc result;
	result.re = a0.re + t * (a1.re + t * (a2.re + t * a3.re));
	result.im = a0.im + t * (a1.im + t * (a2.im + t * a3.im));
	return result;
}