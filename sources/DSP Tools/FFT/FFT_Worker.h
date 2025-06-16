#pragma once

#include <vector>
#include <ipps.h> // Для Ipp32fc, IppStatus и других IPP-типов

// Вспомогательная функция для получения ближайшей степени двойки
// (Обычно это inline или в отдельном утилитарном файле)
inline int GetPow2Closest(int n) {
    if (n <= 0) return 0;
    int power = 1;
    while (power < n) {
        power <<= 1;
    }
    return power;
}

// Вспомогательные функции для обмена половинами данных
// Требуется только после прямого БПФ для центрирования спектра
void SwapHalfes(const Ipp32fc* input_data, Ipp32fc* output_data, int data_size);
void SwapHalfes(const std::vector<Ipp32fc>& input_data, std::vector<Ipp32fc>& output_data);
void SwapHalfes_I(std::vector<Ipp32fc>& input_data); // In-place swap

class FFT_Worker {
public:
    // Конструктор по умолчанию
    FFT_Worker();

    // Деструктор (векторы автоматически управляют памятью)
    ~FFT_Worker();

    // Инициализация FFT_Worker.
    // initial_data_size: Предполагаемый размер данных для БПФ.
    //      Класс найдет ближайшую степень двойки для этого размера.
    // swap_output_on_forward: Если true, выполняется обмен половинами после прямого БПФ.
    bool Init(int initial_data_size, const bool swap_output_on_forward = true);

    // Сброс состояния FFT_Worker, очистка буферов и освобождение памяти.
    void Reset();

    // Выполнение прямого БПФ.
    // Если размер входных данных не соответствует инициализированному размеру БПФ,
    // происходит повторная инициализация.
    bool ForwardFFT(const std::vector<Ipp32fc>& input_data, std::vector<Ipp32fc>& output_data);
    // Выполнение прямого БПФ для C-массивов.
    bool ForwardFFT(const Ipp32fc* input_data, int input_data_size, Ipp32fc* output_data, int output_data_size);

    // Выполнение обратного БПФ.
    // Если размер входных данных не соответствует инициализированному размеру БПФ,
    // происходит повторная инициализация.
    bool InverseFFT(const std::vector<Ipp32fc>& input_data, std::vector<Ipp32fc>& output_data);
    // Выполнение обратного БПФ для C-массивов.
    bool InverseFFT(const Ipp32fc* input_data, int input_data_size, Ipp32fc* output_data, int output_data_size);

private:
    int fft_size_;                      // Размер БПФ (ближайшая степень 2)
    int fft_order_;                     // Порядок БПФ (log2(fft_size_))
    bool swap_output_on_forward_;       // Флаг: выполнять ли обмен половинами после прямого БПФ

    // Указатель на структуру спецификации FFT.
    // Ссылается на данные внутри p_fft_spec_mem_.
    IppsFFTSpec_C_32fc* pFFTSpec_;      
    
    // Буферы, управляемые std::vector для автоматического выделения/освобождения
    // и динамического управления размером.
    std::vector<Ipp8u> p_fft_spec_mem_;     // Память для структуры спецификации FFT
    std::vector<Ipp8u> temp_init_buffer_;   // Временный буфер для ippsFFTInit_C_32fc
    std::vector<Ipp8u> fft_work_buf_;       // Рабочий буфер для ippsFFTForw/Inv_CToC_32fc
    
    // Временные буферы для операции обмена половинами.
    // Выделяются один раз при инициализации для минимизации постоянных аллокаций.
    std::vector<Ipp32fc> temp_swap_buffer_; 
};