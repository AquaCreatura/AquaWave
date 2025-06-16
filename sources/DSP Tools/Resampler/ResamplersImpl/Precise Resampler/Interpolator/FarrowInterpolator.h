#ifndef FARROWRESAMPLER_H
#define FARROWRESAMPLER_H

#include <ipps.h>
#include <vector>
#include <array>
#include <cmath>

class FarrowInterpolator {
public:
    FarrowInterpolator();
    void SetResampleRatio(const double resample_ratio);
    void process(const Ipp32fc* input, size_t input_size, std::vector<Ipp32fc>& output);
    void reset();

private:
    struct FarrowCoeffs {
        Ipp32fc a0, a1, a2, a3;
        FarrowCoeffs(const Ipp32fc* samples);
        Ipp32fc interpolate(float t) const;
    };

    double ratio_;
    double virtual_index_ = 0.0; // Текущая позиция в виртуальных отсчётах
    
    // Хранит последние 3 отсчёта предыдущего блока
    std::array<Ipp32fc, 3> prev_samples_; 
    size_t prev_samples_count_ = 0;
    bool first_block_ = true;
};

#endif // FARROWRESAMPLER_H