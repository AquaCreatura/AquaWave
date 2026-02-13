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
    // Мило проверяем, чтобы входная частота была положительной
    if (base_samplerate <= 0)
        return false;

    // Нежно сохраняем входную и выходную частоты
    input_rate_ = base_samplerate;
    output_rate_ = res_samplerate;
    // Любовно вычисляем соотношение для пересчёта
    resample_ratio_ = static_cast<double>(output_rate_) / input_rate_;

    // Бережно берём длину фильтра из настроек
    const int filter_len = settings_.filter_length;

    // Создаём уютный вектор для коэффициентов фильтра
    std::vector<Ipp32fc> taps(filter_len);
    const double filter_koeff = settings_.filter_koeff;
    // С любовью проектируем фильтр с окошком Блэкмана
    const double window_cutoff = 0.5 * ((resample_ratio_ > 1) ? filter_koeff : filter_koeff * resample_ratio_);
    if(!GenerateWindowKoeffs(window_cutoff, filter_len, ippWinBlackman, taps))
        return false;
    if(settings_.need_norm_power)
    {
        const Ipp32fc norm_koeff = {float(1 / std::pow(resample_ratio_, 0.5)) , 0};
        ippsMulC_32fc_I(norm_koeff, taps.data(), taps.size());
    }
    // Дружелюбно определяем размеры для спецификации и буфера
    int spec_size = 0, buf_size = 0;
    IppStatus status = ippsFIRSRGetSize(filter_len, ipp32fc, &spec_size, &buf_size);
    // Если что-то пошло не так, ласково возвращаем false
    if(status != ippStsNoErr) 
        return false;
    
   

    // Аккуратно освобождаем старые ресурсы, если они были
    Clear();

    // С заботой выделяем память для спецификации фильтра и буфера
    fir_spec_ = reinterpret_cast<IppsFIRSpec_32fc*>(ippsMalloc_8u(spec_size));
    fir_buf_ = ippsMalloc_8u(buf_size);

    // Нежно инициализируем FIR-фильтр с нашими милыми коэффициентами
    status = ippsFIRSRInit_32fc(taps.data(), filter_len, ippAlgDirect, fir_spec_);
    // Если что-то не получилось, заботливо очищаем и уходим
    if (status != ippStsNoErr) 
    {
        Clear();
        return false;
    }

    // С теплом заполняем буфер задержки нуликами
    delay_line_.resize(filter_len, {0.0f, 0.0f});

    // Радостно отмечаем, что всё готово
    initialized_ = true;
    // Ласково настраиваем интерполятор на нужное соотношение
    interpolator_.SetResampleRatio(1 / resample_ratio_);
    // С улыбкой возвращаем успех
    return true;
}

bool PreciseResampler::ProcessData(const Ipp32fc* passed_data, size_t data_size, std::vector<Ipp32fc>& res_data) {
    if (!initialized_ || !passed_data || data_size == 0)
        return false;

    // Вычисление ожидаемого размера выходных данных
    size_t output_size = static_cast<size_t>(data_size * resample_ratio_ + 0.5);

    // Ресайз вектора для хранения отфильтрованных данных
    filtered_vec_.resize(data_size);

    // Применение FIR-фильтра к входным данным с использованием буфера задержки
    IppStatus status = ippsFIRSR_32fc(passed_data, filtered_vec_.data(), static_cast<int>(data_size),
                                      fir_spec_, delay_line_.data(), delay_line_.data(), fir_buf_);
    if (status != ippStsNoErr) {
        res_data.clear();
        return false;
    }

    // Интерполяция отфильтрованных данных для достижения требуемой частоты дискретизации
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