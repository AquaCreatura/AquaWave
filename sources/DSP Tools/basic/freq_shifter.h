#pragma once
#ifndef BASIC_DSP_TOOLS
#define BASIC_DSP_TOOLS
#include <ipps.h>
namespace aqua_dsp_tools
{

// Класс FrequencyShifter осуществляет сдвиг входных комплексных данных на заданную частоту.
// Инициализация производится на основе базовой несущей и целевой несущей.
// Важно, чтобы между вызовами обработки сохранялась фаза для обеспечения непрерывности сигнала.
class FrequencyShifter 
{
public:
    // Конструктор.
    // baseCarrierHz  - базовая несущая (в Гц) входного сигнала.
    // targetCarrierHz- несущая (в Гц), которую необходимо получить на выходе.
    // sampleRateHz   - частота дискретизации (в Гц).
    FrequencyShifter();
    void Init       (double baseCarrierHz, double targetCarrierHz, double sampleRateHz);
    // Деструктор.
    ~FrequencyShifter();

    // Обрабатывает блок данных и записывает результат в отдельный буфер.
    // inputData  - указатель на входной массив комплексных данных.
    // outputData - указатель на выходной массив, куда будут записаны сдвинутые данные.
    // blockSize  - количество элементов в блоке.
    // Возвращает true, если обработка прошла успешно.
    bool ProcessBlock(const Ipp32fc* inputData, Ipp32fc* outputData, int blockSize);

    // Обрабатывает блок данных "на месте" (in-place): сдвинутые данные записываются поверх исходных.
    // data      - указатель на массив комплексных данных, который будет модифицирован.
    // blockSize - количество элементов в блоке.
    // Возвращает true, если обработка прошла успешно.
    bool ProcessBlockInPlace(Ipp32fc* data, int blockSize);

private:
    Ipp32f phase_;    // Текущая фаза генератора тона для обеспечения непрерывности между блоками.
    Ipp32f tone_freq_; // Частота тона для сдвига в радианах на отсчет: вычисляется по разнице целевой и базовой несущей.
    Ipp32f tone_magn_; // Амплитуда тона, обычно равна 1.0.
    std::vector<Ipp32fc> tone_vec_; //Непосредственно сама комплексная синусоида
};










}
#endif // !BASIC_DSP_TOOLS



