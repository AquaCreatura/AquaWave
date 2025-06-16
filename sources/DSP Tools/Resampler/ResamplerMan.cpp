#include <algorithm>
#include <corecrt_math_defines.h>
#include "ResamplerMan.h"
#include "ResamplersImpl/Precise Resampler/PreciseResampler.h"
#include "ResamplersImpl/MR Resampler/MultiRateResampler.h"
constexpr double max_error = 1.e-3;
using namespace aqua_resampler;

// ����������� ��������� �����������.
// ����� ��������������� ������� �������� �������� ����������.
ResamplerManager::ResamplerManager() 
{
    settings_.filter_length = 512; // ����� ������� �� ���������.
    settings_.max_denom = 100;       // ������������ �������� ����������� ��� ������������� ���������.
}

// ���������� ��������� �����������.
// ����������� ��� ���������� ������� ��� ����������� �������.
ResamplerManager::~ResamplerManager() 
{
    FreeResources();
}

// ����� ������������� ����������.
// base_params  - ������� ��������� ���������, ���������� ������� ������������� ��������� �������.
// target_params- ��������� �������� �������, ������� ������������� �������� ����� ���� ��������.
// precise      - ����, ����������� �� ������������� ������� ������� ������������� �����������.
// ���������� true, ���� ������������� ������ �������, ����� false.
bool ResamplerManager::Init(const ProcessingParams base_params, ProcessingParams& target_params, bool precise) 
{
    // ����������� ����� ���������� �������, ���� ������� ����.
    FreeResources();

    const int64_t base_rate = base_params.samplerate_hz;   // �������� ������� �������������.
    int64_t target_rate = target_params.samplerate_hz;       // �������� ������� �������������.

    // ��������� ������������ ������� ����������: ������� ������ ���� ��������������.
    if (base_rate <= 0 || target_rate <= 0) return false;
    // ���� �������� � ������� ������� �����, ���������� �� ���������.
    if (base_rate == target_rate) return true;

    // ��������� ��������� ������� ������� � �������.
    const double target_ratio = static_cast<double>(target_rate) / base_rate;

    try 
    {
        std::pair<int64_t, int64_t> pq; // ���� ��� ������������� ������������������� ��������� � ���� �����.
        bool use_multirate = false;      // ����, ����������� �� ������������� ������������� ���������������� �����������.

        if (precise && (target_ratio < 1.f))  //������������ ������ � ������� MR, �.�. ����� ��������� �������������� �������
        {
            // ���� ��������� ������ ����������, ������� �����, ����������� ������������ � target_ratio,
            // ��� ���� �������� ����� �� ����� ���� ������ target_ratio.
            pq = FindBestFraction(target_ratio, settings_.max_denom, true);
            const double approx_ratio = static_cast<double>(pq.first) / pq.second;
            // ��������� ������� ����� ������������������ � ������� ����������.
            const double diff = std::abs(approx_ratio - target_ratio);
            // ���� ������� �� ��������� 0.001, �������, ��� ����� ������������ ��������������� ����������.
            use_multirate = (diff <= max_error);
        } 
        else 
        {
            // ���� �������� �� �������� ���������, �������������� ��������� � ������� ����������.
            pq = FindBestFraction(target_ratio, settings_.max_denom, true);
            use_multirate = true;
        }

        int64_t new_target_rate; // ����� ������� �������������, ����������� �� ������ ������������������� ���������.
        if (use_multirate) 
        {
            int64_t numerator;
            numerator = base_rate * pq.first; // ��������� ��������� ������ ���������.            
            
            new_target_rate = numerator / pq.second; // ��������� ����� ������� �������������.
            // ������� ������ ���������� ��� ���������������� �����������.
            resampler_ = std::make_unique<PreciseResampler>(); //MultiRateResampler
        } 
        else 
        {
            new_target_rate = target_rate; // ���� ��������������� ���������� �� ������������, ������� ������� �������� ����������.
            // ������� ������ ���������� ��� ������� �����������.
            resampler_ = std::make_unique<PreciseResampler>(); //PreciseResampler
        }

        // �������������� ��������� � ������� � ����� ������� ��������.
        if (!resampler_->Init(base_rate, new_target_rate)) 
        {
            resampler_.reset(); // ���� ������������� �� �������, ����������� �������.
            return false;
        }

        // ��������� ��������� �������� ������� � ����� �������� �������������.
        target_params.samplerate_hz = new_target_rate;
        // ������������� ��������� ����������.
        resampler_->SetSettings(settings_);
        freq_shifter_.Init(base_params.carrier_hz, target_params.carrier_hz, base_params.samplerate_hz);
        return true;
    } 
    catch (const std::exception& e) 
    {
        // � ������ ���������� ����������� ������� � ���������� false.
        return false;
    }
}

// ����� ��������� ����� ������.
// input_data - ��������� �� ������� ������ ������ (����������� ��������).
// size       - ������ �������� ����� ������.
// ���������� true, ���� ������ ���������� �������, ����� false.
bool ResamplerManager::ProcessBlock(const Ipp32fc* input_data, size_t size) 
{
    // ���������, ��� ��������� � ������� ������ ����������, � ������ ����� ������ ����.
    // ����� �������� ������ � ����� ProcessData ����������, ������� ���������� ��������� � processed_data_.
    if( !resampler_ || !input_data || size <= 0 )
        return false;
    shifted_data_.resize(size);
    freq_shifter_.ProcessBlock(input_data,shifted_data_.data(), size);
    bool res = resampler_->ProcessData(shifted_data_.data(), shifted_data_.size(), processed_data_);
    return res;
}

// ����� ��������� ������������ ������.
// ���������� ������ �� ������, ���������� ������������ ����������� ������.
std::vector<Ipp32fc>& ResamplerManager::GetProcessedData() 
{
    return processed_data_;
}

// ����� ������������ ��������, ������������ ����������� � ������� ������������ ������.
void ResamplerManager::FreeResources() 
{
    if (resampler_) 
    {
        resampler_->Clear(); // ������� ���������� ��������� ����������.
        resampler_.reset();  // ������� ������ ����������.
    }
    processed_data_.clear(); // ������� ������ � ������������� �������.
}




