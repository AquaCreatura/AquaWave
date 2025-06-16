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

void FarrowInterpolator::process(const Ipp32fc* input, size_t input_size, std::vector<Ipp32fc>& output) {
    output.clear();
    if (input_size == 0) return;

    std::array<Ipp32fc, 6> combined{};
    size_t combined_size = 0;

    // Копируем предыдущие отсчёты
    if (prev_samples_count_ > 0) {
        memcpy(combined.data(), prev_samples_.data(), prev_samples_count_ * sizeof(Ipp32fc));
        combined_size = prev_samples_count_;
    }

    // Копируем новые отсчёты (не более 3)
    const size_t new_to_copy = std::min(input_size, static_cast<size_t>(3));
    memcpy(combined.data() + combined_size, input, new_to_copy * sizeof(Ipp32fc));
    combined_size += new_to_copy;

    while (true) {
        const int base_index = static_cast<int>(virtual_index_);
        const float frac = virtual_index_ - base_index;

        const int required_samples = base_index + 4;
        if (required_samples > static_cast<int>(combined_size + input_size - new_to_copy)) {
            break;
        }

        Ipp32fc samples[4] = {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}};
        if (base_index < static_cast<int>(combined_size)) {
            const int copy_pos = std::max(base_index, 0);
            const int elements_available = combined_size - copy_pos;
            const int to_copy = std::min(4, elements_available);

            memcpy(samples, combined.data() + copy_pos, to_copy * sizeof(Ipp32fc));
            if (to_copy < 4 && input_size > 0) {
                memcpy(samples + to_copy, input, std::min(4 - to_copy, int(input_size)) * sizeof(Ipp32fc));
            }
        } else {
            const size_t offset = base_index - combined_size;
            if (offset + 4 > input_size) break;
            memcpy(samples, input + offset, 4 * sizeof(Ipp32fc));
        }

        output.push_back(FarrowCoeffs(samples).interpolate(frac));
        virtual_index_ += ratio_;
    }

    // Сохраняем последние 3 отсчёта
    prev_samples_count_ = std::min(input_size, static_cast<size_t>(3));
    if (prev_samples_count_ > 0) {
        const size_t start_pos = input_size - prev_samples_count_;
        memcpy(prev_samples_.data(), input + start_pos, prev_samples_count_ * sizeof(Ipp32fc));
    }

    virtual_index_ -= (input_size - prev_samples_count_);
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