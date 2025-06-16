#include <algorithm>
#include "MultiRateResampler.h"
using namespace aqua_resampler;

MultiRateResampler::MultiRateResampler() 
    : pSpec_(nullptr), up_factor_(0), down_factor_(0), base_rate_(0) {}

MultiRateResampler::~MultiRateResampler() 
{
    Clear();
}

void aqua_resampler::MultiRateResampler::SetSettings(const ResamplerSettings settings)
{
    settings_ = settings;
}

bool MultiRateResampler::Init(const int64_t base_samplerate, int64_t& res_samplerate) {
if (base_samplerate <= 0 || res_samplerate <= 0) 
        return false;

      // Расчёт коэффициентов ресэмплинга
    const double need_ratio = static_cast<double>(res_samplerate) / base_samplerate;
    const auto up_down= aqua_resampler::FindBestFraction(need_ratio, settings_.max_denom, true);
    up_factor_ = up_down.first;
    down_factor_ = up_down.second;
    const auto resample_koeff = float(down_factor_) / up_factor_;    

    // Корректировка конечной частоты
    res_samplerate = (base_samplerate * up_factor_) / down_factor_;

    // Проектирование фильтра с правильной частотой среза
    //В случае для каких целей используется - Если для интерполяции, то подавляем отзеркаливание, если для децимации, то подавляем отзеркаливание
    const double fc = std::min(1.0/(2*up_factor_), 1.0/(2*down_factor_)); 
    std::vector<Ipp32fc> taps;
    if(!GenerateWindowKoeffs(fc, settings_.filter_length, ippWinBlackman, taps))
        return false;
    if(settings_.need_norm_power)
    {
        const Ipp32fc norm_koeff = {powf(up_factor_ * down_factor_, 0.5), 0};
        ippsMulC_32fc_I(norm_koeff , taps.data(), taps.size());
    }
    // Получение размеров буферов
    int specSize = 0, bufSize = 0;
    IppStatus status = ippsFIRMRGetSize(
        settings_.filter_length, 
        up_factor_, 
        down_factor_, 
        ipp32fc, 
        &specSize, 
        &bufSize
    );
    if (status != ippStsNoErr) 
        return false;

    // Выделение памяти с выравниванием (critical для IPP!)
    spec_buffer_.resize(specSize/sizeof(Ipp8u) + 64);
    work_buffer_.resize(bufSize + 64);
    pSpec_ = (IppsFIRSpec_32fc*)spec_buffer_.data();
    // Инициализация спецификации с правильными фазами
    status = ippsFIRMRInit_32fc(
        taps.data(),                  // коэффициенты фильтра
        settings_.filter_length,     // длина фильтра
        up_factor_,                  0,  // upPhase
        down_factor_,                0,               // downPhase 
        pSpec_// выравнивание 
        
    );
    // Инициализация линии задержки
    delay_line_.resize(settings_.filter_length + up_factor_ - 1, {0,0});
    
    return (status == ippStsNoErr);
}

bool MultiRateResampler::ProcessData(const Ipp32fc* data, const size_t size, std::vector<Ipp32fc>& res_vec) 
{
    if (!pSpec_ || !data || size == 0) 
        return false;
    if(size < down_factor_)
        return true;
    // Расчёт размера выходного буфера согласно документации:
    // out_size = floor((in_size * up + phase)/down)
    const int numIters = static_cast<int>(size) / down_factor_;
    const int outSize = numIters * up_factor_;
    res_vec.resize(outSize);

    IppStatus status = ippsFIRMR_32fc(
        data,                        // входные данные
        res_vec.data(),              // выходной буфер
        numIters,                    // количество входных блоков
        pSpec_, // спецификация
        delay_line_.data(),          // исходное состояние
        delay_line_.data(),          // обновлённое состояние
        work_buffer_.data()          // временный буфер
    );

    /*if(settings_.need_norm_power)
    {
        const Ipp32fc norm_koeff = {powf((float(up_factor_) ), 0.5), 0};
        ippsMulC_32fc_I(norm_koeff , res_vec.data(), res_vec.size());
    }*/
    
    return (status == ippStsNoErr);
}

void MultiRateResampler::Clear() 
{
    pSpec_ = nullptr;

    spec_buffer_.clear();
    spec_buffer_.shrink_to_fit();

    work_buffer_.clear();
    work_buffer_.shrink_to_fit();

    delay_line_.clear();
    delay_line_.shrink_to_fit();


    up_factor_ = down_factor_ = 0;
}