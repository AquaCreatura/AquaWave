// FarrowInterpolator.cpp
#include "FarrowInterpolator.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>

FarrowInterpolator::FarrowInterpolator() : ratio_(1.0), virtual_index_(0.0) {
	tail_.fill({ 0.0f, 0.0f });
}

void FarrowInterpolator::SetResampleRatio(double resample_ratio) {
	if (resample_ratio <= 0.0 || resample_ratio > 200.0) {
		throw std::invalid_argument("Invalid resample ratio");
	}
	ratio_ = resample_ratio;
}

bool FarrowInterpolator::can_interpolate(int base_index, size_t input_size) const {
	// Нужны 4 точки: base-2, base-1, base, base+1

	// base и base+1 должны быть в текущем блоке
	if (base_index < -2 || base_index + 1 >= static_cast<int>(input_size)) {
		return false;
	}

	return true;
}

Ipp32fc FarrowInterpolator::get_sample(int index, const Ipp32fc* input, size_t input_size) const {
	if (index < 0) {
		// Берем из хвоста: индексы -1, -2, -3
		int tail_idx = 4 + index; // -1 -> 1, -2 -> 0, -3 -> -1
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
	// Если блок меньше 3 семплов, копируем все что есть
	size_t copy_count = std::min(input_size, static_cast<size_t>(4));

	for (size_t i = 0; i < copy_count; ++i) {
		tail_[i] = input[input_size - copy_count + i];
	}

	// Если скопировали меньше 3, оставшиеся не трогаем
	// (они уже содержат нужные данные из предыдущих блоков)
}

void FarrowInterpolator::process(
	const Ipp32fc* input,
	size_t input_size,
	std::vector<Ipp32fc>& output)
{
	output.clear();
	if (input_size == 0) return;

	output.reserve(static_cast<size_t>(input_size / ratio_) + 10);
	double was_virtual = virtual_index_;
	while (true) {
		int base = static_cast<int>(floor(virtual_index_));
		float t = static_cast<float>(virtual_index_ - base);

		if (!can_interpolate(base, input_size)) {
			break;
		}

		Ipp32fc points[4];
		points[0] = get_sample(base - 2, input, input_size);
		points[1] = get_sample(base - 1, input, input_size);
		points[2] = get_sample(base, input, input_size);
		points[3] = get_sample(base + 1, input, input_size);

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
	const Ipp32fc& y_m2 = samples[0];
	const Ipp32fc& y_m1 = samples[1];
	const Ipp32fc& y0 = samples[2];
	const Ipp32fc& y1 = samples[3];

	const float sixth = 1.0f / 6.0f;
	const float half = 0.5f;

	Ipp32fc a3 = {
		(y1.re - y_m2.re) * sixth + (y_m1.re - y0.re) * half,
		(y1.im - y_m2.im) * sixth + (y_m1.im - y0.im) * half
	};

	a1 = {
		(y1.re - y_m1.re) * half - a3.re,
		(y1.im - y_m1.im) * half - a3.im
	};

	a2 = {
		y1.re - y0.re - a1.re - a3.re,
		y1.im - y0.im - a1.im - a3.im
	};

	this->a3 = a3;
	a0 = y0;
}

Ipp32fc FarrowInterpolator::FarrowCoeffs::interpolate(float t) const {
	Ipp32fc result;
	result.re = a0.re + t * (a1.re + t * (a2.re + t * a3.re));
	result.im = a0.im + t * (a1.im + t * (a2.im + t * a3.im));
	return result;
}