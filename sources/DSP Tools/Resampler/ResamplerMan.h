#pragma once
#include <memory>
#include "ResamplersImpl/ResamperInterface.h"
#include "../basic/freq_shifter.h"
namespace aqua_resampler
{

// Структура ProcessingParams содержит параметры обработки сигнала.
struct ProcessingParams 
{
    int64_t samplerate_hz; // Базовая частота дискретизации сигнала в герцах.
    double  carrier_hz;    // Частота несущего сигнала в герцах.
};

// Класс ResamplerManager управляет процессом ресэмплинга (изменения частоты дискретизации) сигнала.
class ResamplerManager 
{
public:
    // Конструктор: инициализирует объект менеджера ресэмплинга.
    ResamplerManager();

    // Деструктор: освобождает все используемые ресурсы.
    ~ResamplerManager();

    // Метод инициализации ресэмплера.
    // Параметры:
    //   base_params  - исходные параметры обработки (например, базовая частота дискретизации).
    //   target_params- параметры целевого сигнала, частота которого может измениться в процессе ресэмплинга.
    //   precise      - флаг, указывающий на необходимость точного расчёта коэффициентов ресэмплинга.
    // Возвращает true, если инициализация прошла успешно, иначе false.
    bool Init(const ProcessingParams base_params, ProcessingParams& target_params, bool precise);

    // Метод обработки блока входных данных.
    // input_data - указатель на массив входных комплексных данных.
    // size       - количество элементов входного массива.
    // Возвращает true, если данные обработаны успешно, иначе false.
    bool ProcessBlock(const Ipp32fc* input_data, size_t size);

    // Метод для получения обработанных данных.
    // Возвращает ссылку на вектор, содержащий комплексные данные после ресэмплинга.
    std::vector<Ipp32fc>& GetProcessedData();

    // Метод для освобождения всех ресурсов, используемых ресэмплером и внутренними буферами.
    void FreeResources();

private:
    // Вектор, содержащий обработанные (ресэмплированные) данные.
    std::vector<Ipp32fc>                processed_data_;
    // Указатель на объект интерфейса ресэмплера (например, многоскоростной или точный ресэмплер).
    std::unique_ptr<ResamplerInterface> resampler_;

    // Настройки ресэмплера, включающие параметры фильтра и другие настройки.
    ResamplerSettings                   settings_;

    // Класс, обеспечивающий свдиг данных
    aqua_dsp_tools::FrequencyShifter    freq_shifter_;
    std::vector<Ipp32fc>                shifted_data_;
};

}