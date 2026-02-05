#include "FarrowInterpolator.h"
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <array>
#include <cassert>

FarrowInterpolator::FarrowInterpolator() : ratio_(1.0), virtual_index_(0.0) {
	prev_samples_.fill({ 0.0f, 0.0f });
}

void FarrowInterpolator::SetResampleRatio(double resample_ratio) {
	if (resample_ratio <= 0.0 || resample_ratio > 10.0) {
		throw std::invalid_argument("Resampling ratio must be in (0.0, 10.0]");
	}
	ratio_ = resample_ratio;
}

void FarrowInterpolator::process(
	const Ipp32fc* input,
	size_t input_size,
	std::vector<Ipp32fc>& output)
{
	output.clear();
	if (input_size == 0) return;

	// Функция для получения семпла с учётом предыдущих
	auto get_sample = [&](int idx) -> Ipp32fc {
		if (idx < 0) {
			// Берём из предыдущих семплов
			int prev_idx = static_cast<int>(prev_samples_count_) + idx;
			return (prev_idx >= 0) ? prev_samples_[prev_idx] : Ipp32fc{ 0.0f, 0.0f };
		}

		// Берём из текущего ввода
		return (static_cast<size_t>(idx) < input_size) ?
			input[idx] : Ipp32fc{ 0.0f, 0.0f };
	};

	// Генерируем выходные семплы пока можем
	while (true) {
		const int base_index = static_cast<int>(std::floor(virtual_index_));
		const float t = static_cast<float>(virtual_index_ - base_index);

		// Проверяем, доступны ли все 4 точки для интерполяции
		if (base_index - 2 < -static_cast<int>(prev_samples_count_)) break;
		if (base_index + 1 >= static_cast<int>(input_size)) break;

		// Получаем четыре точки для интерполяции
		Ipp32fc samples[4];
		samples[0] = get_sample(base_index - 2); // y[-2]
		samples[1] = get_sample(base_index - 1); // y[-1]
		samples[2] = get_sample(base_index);     // y[0]
		samples[3] = get_sample(base_index + 1); // y[1]

												 // Вычисляем коэффициенты и интерполируем
		const FarrowCoeffs coeffs(samples);
		output.push_back(coeffs.interpolate(t));

		// Переходим к следующему выходному семплу
		virtual_index_ += ratio_;
	}

	// Определяем сколько входных семплов было использовано
	const int used_samples = static_cast<int>(virtual_index_) + 1;

	// Сохраняем последние семплы для следующего блока
	save_prev_samples(input, input_size, used_samples);

	// Сдвигаем виртуальный индекс для следующего блока
	virtual_index_ -= static_cast<double>(used_samples);
}

void FarrowInterpolator::save_prev_samples(const Ipp32fc* input, size_t input_size, int used_samples) {
	// Сколько семплов сохранить (максимум 3)
	const int samples_to_save = std::min(3, std::min(used_samples, static_cast<int>(input_size)));

	if (samples_to_save > 0) {
		// Сохраняем последние samples_to_save семплов из использованных
		for (int i = 0; i < samples_to_save; ++i) {
			const int src_idx = used_samples - samples_to_save + i;
			if (src_idx >= 0 && static_cast<size_t>(src_idx) < input_size) {
				prev_samples_[i] = input[src_idx];
			}
		}
		prev_samples_count_ = samples_to_save;
	}
	else if (input_size > 0) {
		// Если не использовали ни одного семпла, сохраняем последние 3 семпла текущего блока
		const size_t samples_to_keep = std::min(input_size, static_cast<size_t>(3));
		for (size_t i = 0; i < samples_to_keep; ++i) {
			prev_samples_[i] = input[input_size - samples_to_keep + i];
		}
		prev_samples_count_ = samples_to_keep;
	}
}

void FarrowInterpolator::reset() {
	virtual_index_ = 0.0;
	prev_samples_count_ = 0;
	prev_samples_.fill({ 0.0f, 0.0f });
}

FarrowInterpolator::FarrowCoeffs::FarrowCoeffs(const Ipp32fc* samples) {
	const Ipp32fc& y_m2 = samples[0];
	const Ipp32fc& y_m1 = samples[1];
	const Ipp32fc& y0 = samples[2];
	const Ipp32fc& y1 = samples[3];

	// Вычисляем коэффициенты кубического интерполятора Фарроу
	// a3 = (y1 - y_m2)/6 + (y_m1 - y0)/2
	const Ipp32fc a3_tmp = {
		(y1.re - y_m2.re) * (1.0f / 6.0f) + (y_m1.re - y0.re) * 0.5f,
		(y1.im - y_m2.im) * (1.0f / 6.0f) + (y_m1.im - y0.im) * 0.5f
	};

	// a1 = (y1 - y_m1)/2 - a3
	a1 = {
		(y1.re - y_m1.re) * 0.5f - a3_tmp.re,
		(y1.im - y_m1.im) * 0.5f - a3_tmp.im
	};

	// a2 = y1 - y0 - a1 - a3
	a2 = {
		y1.re - y0.re - a1.re - a3_tmp.re,
		y1.im - y0.im - a1.im - a3_tmp.im
	};

	a3 = a3_tmp;
	a0 = y0;
}

Ipp32fc FarrowInterpolator::FarrowCoeffs::interpolate(float t) const {
	// Схема Горнера для вычисления кубического полинома
	// p(t) = a0 + t*(a1 + t*(a2 + t*a3))
	const Ipp32fc inner = {
		a2.re + t * a3.re,
		a2.im + t * a3.im
	};

	const Ipp32fc middle = {
		a1.re + t * inner.re,
		a1.im + t * inner.im
	};

	return {
		a0.re + t * middle.re,
		a0.im + t * middle.im
	};
}