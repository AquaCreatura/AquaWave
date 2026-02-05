#include <corecrt_math_defines.h>
#include <vector>
#include "freq_shifter.h"
using namespace aqua_dsp_tools;

// Конструктор. Вычисляет частоту тона для сдвига (в радианах на отсчет)
// по формуле: toneFreq = 2π * (targetCarrier - baseCarrier) / sampleRate.
FrequencyShifter::FrequencyShifter()
    : phase_(0.0f), tone_magn_(1.0f)
{
    
}
void FrequencyShifter::Init(double baseCarrierHz, double targetCarrierHz, double sampleRateHz)
{
    // Вычисляем разницу частот (может быть положительной или отрицательной)
    double freqShift = targetCarrierHz - baseCarrierHz;

    // Вычисляем относительную частоту тона (rFreq) как абсолютное значение
    tone_freq_ = -1 * static_cast<Ipp32f>(std::abs(freqShift) / sampleRateHz);
	if (tone_freq_ < 0) tone_freq_ += 1;
    // Инициализируем амплитуду (по умолчанию 1.0)
    tone_magn_ = 1.0f; 

    // Инициализируем начальную фазу
    phase_ = 0.0f;
}

// Деструктор (нет динамически выделенных ресурсов, поэтому ничего не освобождаем)
FrequencyShifter::~FrequencyShifter() {
    // Пусто, всё чистенько
}

// Обработка блока данных с записью результата в отдельный буфер
bool FrequencyShifter::ProcessBlock(const Ipp32fc* inputData, Ipp32fc* outputData, int blockSize)
{
    if (!inputData || !outputData || blockSize <= 0)
        return false;
	if (tone_freq_ == 0) {
		ippsCopy_32fc(inputData, outputData, blockSize);
		return true;
	}
        
    // Проверяем и подгоняем размер буфера для тона
    if (tone_vec_.size() != static_cast<size_t>(blockSize)) {
        tone_vec_.resize(blockSize);
    }

    // Генерируем комплексный тон с сохранением фазы
    IppStatus status = ippsTone_32fc(tone_vec_.data(), blockSize, tone_magn_, tone_freq_, &phase_, ippAlgHintFast);
    if (status != ippStsNoErr) {
        return false;
    }


    // Умножаем входной сигнал на тон и записываем результат
    status = ippsMul_32fc(inputData, tone_vec_.data(), outputData, blockSize);
    return (status == ippStsNoErr);
}

// Обработка блока данных "на месте" (in-place)
bool FrequencyShifter::ProcessBlockInPlace(Ipp32fc* data, int blockSize)
{
    if (!data || blockSize <= 0)
        return false;

    // Создаём временный буфер для тона
    std::vector<Ipp32fc> tone(blockSize);

    // Генерируем тон с сохранением фазы
    IppStatus status = ippsTone_32fc(tone.data(), blockSize, tone_magn_, tone_freq_, &phase_, ippAlgHintFast);
    if (status != ippStsNoErr) {
        return false;
    }
    // Умножаем сигнал на тон "на месте"
    status = ippsMul_32fc_I(tone.data(), data, blockSize);
    return (status == ippStsNoErr);
}