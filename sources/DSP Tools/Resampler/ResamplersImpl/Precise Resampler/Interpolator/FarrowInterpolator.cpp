#include "FarrowInterpolator.h"
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <array>

FarrowInterpolator::FarrowInterpolator()
{
};

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
    if (input_size == 0)
        return;

    auto get_sample = [&](int idx) -> Ipp32fc {
        // idx < 0  берём из prev_samples_
        if (idx < 0) {
            const int p = static_cast<int>(prev_samples_count_) + idx;
            if (p >= 0)
                return prev_samples_[p];
            return {0.0f, 0.0f};
        }

        // idx >= 0  из текущего input
        if (idx < static_cast<int>(input_size))
            return input[idx];

        return {0.0f, 0.0f};
    };

    while (true) {
        const int base_index = static_cast<int>(std::floor(virtual_index_));
        const float t = static_cast<float>(virtual_index_ - base_index);

        // Проверяем доступность y(-2), y(-1), y0, y1
        if (base_index - 2 < -static_cast<int>(prev_samples_count_))
            break;
        if (base_index + 1 >= static_cast<int>(input_size))
            break;

        Ipp32fc samples[4];
        samples[0] = get_sample(base_index - 2);
        samples[1] = get_sample(base_index - 1);
        samples[2] = get_sample(base_index);
        samples[3] = get_sample(base_index + 1);

        output.push_back(
            FarrowCoeffs(samples).interpolate(t)
        );

        virtual_index_ += ratio_;
    }

    // Сохраняем последние 3 отсчёта для следующего вызова
    prev_samples_count_ = std::min(input_size, static_cast<size_t>(3));
    for (size_t i = 0; i < prev_samples_count_; ++i) {
        prev_samples_[i] = input[input_size - prev_samples_count_ + i];
    }

    // Сдвигаем виртуальный индекс в новую систему координат
    virtual_index_ -= static_cast<double>(input_size);
}


void FarrowInterpolator::reset() {
    virtual_index_ = 0.0;
    prev_samples_count_ = 0;
    prev_samples_.fill({0.0f, 0.0f});
}

FarrowInterpolator::FarrowCoeffs::FarrowCoeffs(const Ipp32fc* samples) {
    const Ipp32fc& y_m2 = samples[0];
    const Ipp32fc& y_m1 = samples[1];
    const Ipp32fc& y0 = samples[2];
    const Ipp32fc& y1 = samples[3];

    const Ipp32fc temp1 = {
        (y1.re - y_m2.re) / 6.0f + (y_m1.re - y0.re) / 2.0f,
        (y1.im - y_m2.im) / 6.0f + (y_m1.im - y0.im) / 2.0f
    };

    a3 = temp1;
    a1 = {
        (y1.re - y_m1.re) / 2.0f - temp1.re,
        (y1.im - y_m1.im) / 2.0f - temp1.im
    };
    a2 = {
        y1.re - y0.re - a1.re - temp1.re,
        y1.im - y0.im - a1.im - temp1.im
    };
    a0 = y0;
}

Ipp32fc FarrowInterpolator::FarrowCoeffs::interpolate(float t) const {
    if (t < 0.0f || t > 1.0f) {
        throw std::out_of_range("Interpolation parameter t must be in [0, 1]");
    }
    const float t2 = t * t;
    const float t3 = t2 * t;
    return {
        a0.re + a1.re * t + a2.re * t2 + a3.re * t3,
        a0.im + a1.im * t + a2.im * t2 + a3.im * t3
    };
}