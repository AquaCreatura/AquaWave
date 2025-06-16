#include "PreciseResampler.h"
using namespace aqua_resampler;

PreciseResampler::PreciseResampler()
{
}

PreciseResampler::~PreciseResampler()
{
    Clear();
}

void aqua_resampler::PreciseResampler::SetSettings(const ResamplerSettings settings)
{
    settings_ = settings;
}

bool PreciseResampler::Init(int64_t base_samplerate, int64_t& res_samplerate) 
{
    // ���� ���������, ����� ������� ������� ���� �������������
    if (base_samplerate <= 0)
        return false;

    // ����� ��������� ������� � �������� �������
    input_rate_ = base_samplerate;
    output_rate_ = res_samplerate;
    // ������� ��������� ����������� ��� ���������
    resample_ratio_ = static_cast<double>(output_rate_) / input_rate_;

    // ������� ���� ����� ������� �� ��������
    const int filter_len = settings_.filter_length;

    // ������ ������ ������ ��� ������������� �������
    std::vector<Ipp32fc> taps(filter_len);
    const double filter_koeff = 0.99;
    // � ������� ����������� ������ � ������� ��������
    const double window_cutoff = 0.5 * ((resample_ratio_ > 1) ? filter_koeff : filter_koeff * resample_ratio_);
    if(!GenerateWindowKoeffs(window_cutoff, filter_len, ippWinBlackman, taps))
        return false;
    if(settings_.need_norm_power)
    {
        const Ipp32fc norm_koeff = {float(1 / std::pow(resample_ratio_, 0.5)) , 0};
        ippsMulC_32fc_I(norm_koeff, taps.data(), taps.size());
    }
    // ���������� ���������� ������� ��� ������������ � ������
    int spec_size = 0, buf_size = 0;
    IppStatus status = ippsFIRSRGetSize(filter_len, ipp32fc, &spec_size, &buf_size);
    // ���� ���-�� ����� �� ���, ������� ���������� false
    if(status != ippStsNoErr) 
        return false;
    
   

    // ��������� ����������� ������ �������, ���� ��� ����
    Clear();

    // � ������� �������� ������ ��� ������������ ������� � ������
    fir_spec_ = reinterpret_cast<IppsFIRSpec_32fc*>(ippsMalloc_8u(spec_size));
    fir_buf_ = ippsMalloc_8u(buf_size);

    // ����� �������������� FIR-������ � ������ ������ ��������������
    status = ippsFIRSRInit_32fc(taps.data(), filter_len, ippAlgDirect, fir_spec_);
    // ���� ���-�� �� ����������, ��������� ������� � ������
    if (status != ippStsNoErr) 
    {
        Clear();
        return false;
    }

    // � ������ ��������� ����� �������� ��������
    delay_line_.resize(filter_len, {0.0f, 0.0f});

    // �������� ��������, ��� �� ������
    initialized_ = true;
    // ������� ����������� ������������ �� ������ �����������
    interpolator_.SetResampleRatio(1 / resample_ratio_);
    // � ������� ���������� �����
    return true;
}

bool PreciseResampler::ProcessData(const Ipp32fc* passed_data, size_t data_size, std::vector<Ipp32fc>& res_data) {
    if (!initialized_ || !passed_data || data_size == 0)
        return false;

    // ���������� ���������� ������� �������� ������
    size_t output_size = static_cast<size_t>(data_size * resample_ratio_ + 0.5);

    // ������ ������� ��� �������� ��������������� ������
    filtered_vec_.resize(data_size);

    // ���������� FIR-������� � ������� ������ � �������������� ������ ��������
    IppStatus status = ippsFIRSR_32fc(passed_data, filtered_vec_.data(), static_cast<int>(data_size),
                                      fir_spec_, delay_line_.data(), delay_line_.data(), fir_buf_);
    if (status != ippStsNoErr) {
        res_data.clear();
        return false;
    }

    // ������������ ��������������� ������ ��� ���������� ��������� ������� �������������
    interpolator_.process(filtered_vec_.data(), filtered_vec_.size(), res_data);

    return true;
}

void PreciseResampler::Clear() {
    if (fir_spec_) {
        ippsFree(fir_spec_);
        fir_spec_ = nullptr;
    }
    if (fir_buf_) {
        ippsFree(fir_buf_);
        fir_buf_ = nullptr;
    }
    delay_line_.clear();
    initialized_ = false;
}