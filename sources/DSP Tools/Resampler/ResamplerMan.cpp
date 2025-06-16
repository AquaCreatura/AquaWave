#include <algorithm>
#include <corecrt_math_defines.h>
#include "ResamplerMan.h"
#include "ResamplersImpl/Precise Resampler/PreciseResampler.h"
#include "ResamplersImpl/MR Resampler/MultiRateResampler.h"
constexpr double max_error = 1.e-3;
using namespace aqua_resampler;

// Конструктор менеджера ресэмплинга.
// Здесь устанавливаются базовые значения настроек ресэмплера.
ResamplerManager::ResamplerManager() 
{
    settings_.filter_length = 512; // Длина фильтра по умолчанию.
    settings_.max_denom = 100;       // Максимальное значение знаменателя для аппроксимации отношения.
}

// Деструктор менеджера ресэмплинга.
// Освобождает все выделенные ресурсы при уничтожении объекта.
ResamplerManager::~ResamplerManager() 
{
    FreeResources();
}

// Метод инициализации ресэмплера.
// base_params  - базовые параметры обработки, содержащие частоту дискретизации исходного сигнала.
// target_params- параметры целевого сигнала, частота дискретизации которого может быть изменена.
// precise      - флаг, указывающий на необходимость точного расчёта коэффициентов ресэмплинга.
// Возвращает true, если инициализация прошла успешно, иначе false.
bool ResamplerManager::Init(const ProcessingParams base_params, ProcessingParams& target_params, bool precise) 
{
    // Освобождаем ранее выделенные ресурсы, если таковые были.
    FreeResources();

    const int64_t base_rate = base_params.samplerate_hz;   // Исходная частота дискретизации.
    int64_t target_rate = target_params.samplerate_hz;       // Желаемая частота дискретизации.

    // Проверяем корректность входных параметров: частоты должны быть положительными.
    if (base_rate <= 0 || target_rate <= 0) return false;
    // Если исходная и целевая частоты равны, ресэмплинг не требуется.
    if (base_rate == target_rate) return true;

    // Вычисляем отношение целевой частоты к базовой.
    const double target_ratio = static_cast<double>(target_rate) / base_rate;

    try 
    {
        std::pair<int64_t, int64_t> pq; // Пара для представления аппроксимированного отношения в виде дроби.
        bool use_multirate = false;      // Флаг, указывающий на необходимость использования многоскоростного ресэмплинга.

        if (precise && (target_ratio < 1.f))  //Интерполяция только с помощью MR, т.к. иначе возникают отзеркаливание спектра
        {
            // Если требуется точный ресэмплинг, находим дробь, максимально приближенную к target_ratio,
            // при этом итоговая дробь не может быть меньше target_ratio.
            pq = FindBestFraction(target_ratio, settings_.max_denom, true);
            const double approx_ratio = static_cast<double>(pq.first) / pq.second;
            // Вычисляем разницу между аппроксимированным и целевым отношением.
            const double diff = std::abs(approx_ratio - target_ratio);
            // Если разница не превышает 0.001, считаем, что можно использовать многоскоростной ресэмплинг.
            use_multirate = (diff <= max_error);
        } 
        else 
        {
            // Если точность не является критичной, аппроксимируем отношение с помощью округления.
            pq = FindBestFraction(target_ratio, settings_.max_denom, true);
            use_multirate = true;
        }

        int64_t new_target_rate; // Новая частота дискретизации, вычисленная на основе аппроксимированного отношения.
        if (use_multirate) 
        {
            int64_t numerator;
            numerator = base_rate * pq.first; // Вычисляем числитель нового отношения.            
            
            new_target_rate = numerator / pq.second; // Вычисляем новую частоту дискретизации.
            // Создаем объект ресэмплера для многоскоростного ресэмплинга.
            resampler_ = std::make_unique<PreciseResampler>(); //MultiRateResampler
        } 
        else 
        {
            new_target_rate = target_rate; // Если многоскоростной ресэмплинг не используется, целевая частота остается неизменной.
            // Создаем объект ресэмплера для точного ресэмплинга.
            resampler_ = std::make_unique<PreciseResampler>(); //PreciseResampler
        }

        // Инициализируем ресэмплер с базовой и новой целевой частотой.
        if (!resampler_->Init(base_rate, new_target_rate)) 
        {
            resampler_.reset(); // Если инициализация не удалась, освобождаем ресурсы.
            return false;
        }

        // Обновляем параметры целевого сигнала с новой частотой дискретизации.
        target_params.samplerate_hz = new_target_rate;
        // Устанавливаем настройки ресэмплера.
        resampler_->SetSettings(settings_);
        freq_shifter_.Init(base_params.carrier_hz, target_params.carrier_hz, base_params.samplerate_hz);
        return true;
    } 
    catch (const std::exception& e) 
    {
        // В случае исключения освобождаем ресурсы и возвращаем false.
        return false;
    }
}

// Метод обработки блока данных.
// input_data - указатель на входной массив данных (комплексные значения).
// size       - размер входного блока данных.
// Возвращает true, если данные обработаны успешно, иначе false.
bool ResamplerManager::ProcessBlock(const Ipp32fc* input_data, size_t size) 
{
    // Проверяем, что ресэмплер и входные данные существуют, а размер блока больше нуля.
    // Затем передаем данные в метод ProcessData ресэмплера, который записывает результат в processed_data_.
    if( !resampler_ || !input_data || size <= 0 )
        return false;
    shifted_data_.resize(size);
    freq_shifter_.ProcessBlock(input_data,shifted_data_.data(), size);
    bool res = resampler_->ProcessData(shifted_data_.data(), shifted_data_.size(), processed_data_);
    return res;
}

// Метод получения обработанных данных.
// Возвращает ссылку на вектор, содержащий обработанные комплексные данные.
std::vector<Ipp32fc>& ResamplerManager::GetProcessedData() 
{
    return processed_data_;
}

// Метод освобождения ресурсов, используемых ресэмплером и буфером обработанных данных.
void ResamplerManager::FreeResources() 
{
    if (resampler_) 
    {
        resampler_->Clear(); // Очищаем внутреннее состояние ресэмплера.
        resampler_.reset();  // Удаляем объект ресэмплера.
    }
    processed_data_.clear(); // Очищаем вектор с обработанными данными.
}




