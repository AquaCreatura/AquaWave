#pragma once
#include "..\ResamperInterface.h"
#include "..\..\tools\resampler_tools.h"
#include "Interpolator/FarrowInterpolator.h"
namespace aqua_resampler
{

class PreciseResampler : public ResamplerInterface
{
public:
    PreciseResampler();
    virtual ~PreciseResampler() override;

    virtual void SetSettings    (const ResamplerSettings settings                                                  ) override;
    virtual bool Init           (const int64_t base_samplerate, int64_t& res_samplerate                            ) override;
    virtual bool ProcessData    (const Ipp32fc* passed_data, const size_t data_size, std::vector<Ipp32fc>& res_data) override;
    virtual void Clear          () override;
private:
    ResamplerSettings settings_;       // Настройки ресемплера
    int64_t input_rate_;               // Исходная частота дискретизации
    int64_t output_rate_;              // Требуемая частота дискретизации
    double resample_ratio_;            // Коэффициент ресемплинга
    IppsFIRSpec_32fc* fir_spec_;       // Спецификация FIR-фильтра
    Ipp8u* fir_buf_;                   // Временный буфер для фильтрации
    std::vector<Ipp32fc> delay_line_;  // Буфер задержки для сохранения состояния фильтра
    bool initialized_;                 // Флаг инициализации
    FarrowInterpolator interpolator_;  // Интерполятор для изменения частоты дискретизации
    std::vector<Ipp32fc> filtered_vec_;// Вектор для хранения отфильтрованных данных
};

}
