#include "FFT_Worker.h"
#include <cmath>     // Для std::log2
#include <stdexcept> // Для обработки ошибок

// Реализация вспомогательных функций SwapHalfes
// ---------------------------------------------

void SwapHalfes(const Ipp32fc* input_data, Ipp32fc* output_data, int data_size) {
    if (!input_data || !output_data || data_size <= 0) {
        throw std::invalid_argument("SwapHalfes: Invalid input parameters (null pointers or data_size <= 0).");
    }
    int half_size = data_size / 2;
    // Копируем вторую половину в начало выходного буфера
    ippsCopy_32fc(input_data + half_size, output_data, half_size);
    // Копируем первую половину во вторую половину выходного буфера
    ippsCopy_32fc(input_data, output_data + half_size, half_size);
}

void SwapHalfes(const std::vector<Ipp32fc>& input_data, std::vector<Ipp32fc>& output_data) {
    if (input_data.empty() || input_data.size() != output_data.size()) {
        throw std::invalid_argument("SwapHalfes: Input/output vector size mismatch or empty.");
    }
    SwapHalfes(input_data.data(), output_data.data(), static_cast<int>(input_data.size()));
}

void SwapHalfes_I(std::vector<Ipp32fc>& input_data) {
    if (input_data.empty()) {
        throw std::invalid_argument("SwapHalfes_I: Input vector is empty.");
    }
    int data_size = static_cast<int>(input_data.size());
    int half_size = data_size / 2;
    // Используем временный буфер для безопасного обмена на месте
    std::vector<Ipp32fc> temp_buffer(half_size); 
    
    ippsCopy_32fc(input_data.data() + half_size, temp_buffer.data(), half_size);
    ippsCopy_32fc(input_data.data(), input_data.data() + half_size, half_size);
    ippsCopy_32fc(temp_buffer.data(), input_data.data(), half_size);
}

// Реализация методов класса FFT_Worker
// ------------------------------------

FFT_Worker::FFT_Worker() 
    : fft_size_(0), 
      fft_order_(0), 
      swap_output_on_forward_(false),
      pFFTSpec_(nullptr) {
    // Векторы инициализируются по умолчанию пустыми
}

FFT_Worker::~FFT_Worker() {
    // Векторы (std::vector<Ipp8u> и std::vector<Ipp32fc>) автоматически 
    // освобождают свою память при уничтожении объекта FFT_Worker.
    // Нет необходимости в явных вызовах ippsFree или delete.
}

bool FFT_Worker::Init(int initial_data_size, const bool swap_output_on_forward) {
    if (initial_data_size <= 0) {
        throw std::invalid_argument("FFT_Worker::Init: initial_data_size must be greater than 0.");
    }

    const int new_fft_size = GetPow2Closest(initial_data_size);
    const int new_fft_order = static_cast<int>(std::log2(new_fft_size));

    // Если размер БПФ и флаг обмена не изменились, нет необходимости в повторной инициализации
    if (new_fft_size == fft_size_ && swap_output_on_forward == swap_output_on_forward_) {
        return true;
    }

    fft_size_ = new_fft_size;
    fft_order_ = new_fft_order;
    swap_output_on_forward_ = swap_output_on_forward;

    // Режим деления для обратного БПФ.
    // IPP_FFT_DIV_INV_BY_N означает, что ippsFFTInv_CToC_32fc будет делить результат на N.
    // Если вам требуется ручное деление (как в вашем примере), используйте ippAlgHintNone
    // и выполните деление самостоятельно после InverseFFT.
    auto fft_div_mode = IPP_FFT_DIV_BY_SQRTN; 
    IppHintAlgorithm alg_hint = ippAlgHintFast; // Используем hintFast для производительности

    int p_fft_spec_size = 0;
    int p_fft_spec_buf_size = 0;
    int fft_external_buf_size = 0;

    // Получаем размеры буферов, необходимых для IPP FFT
    IppStatus status = ippsFFTGetSize_C_32fc(
        fft_order_,         // Порядок БПФ (log2(N))
        fft_div_mode,
        alg_hint,
        &p_fft_spec_size,
        &p_fft_spec_buf_size,
        &fft_external_buf_size
    );

    if (status != ippStsNoErr) {
        // Возможно, log2(initial_data_size) слишком большой или другие ошибки IPP
        // В реальном коде можно добавить более детальное логирование статуса IPP.
        return false;
    }

    // Изменяем размер внутренних буферов std::vector, если они меньше требуемого.
    // resize() гарантирует, что ёмкость будет достаточной.
    if (fft_work_buf_.size() < static_cast<size_t>(fft_external_buf_size)) {
        fft_work_buf_.resize(fft_external_buf_size);
    }
    if (p_fft_spec_mem_.size() < static_cast<size_t>(p_fft_spec_size)) {
        p_fft_spec_mem_.resize(p_fft_spec_size);
    }
    if (temp_init_buffer_.size() < static_cast<size_t>(p_fft_spec_buf_size)) {
        temp_init_buffer_.resize(p_fft_spec_buf_size);
    }

    // Инициализируем структуру спецификации FFT.
    // pFFTSpec_ будет указывать на начало p_fft_spec_mem_.
    pFFTSpec_ = reinterpret_cast<IppsFFTSpec_C_32fc*>(p_fft_spec_mem_.data());

    status = ippsFFTInit_C_32fc(
        &pFFTSpec_,          // Адрес указателя на спецификацию
        fft_order_,          // Порядок БПФ (log2(N))
        fft_div_mode,
        alg_hint,
        p_fft_spec_mem_.data(), // Буфер для спецификации
        temp_init_buffer_.data() // Временный буфер для инициализации
    );

    if (status != ippStsNoErr) {
        pFFTSpec_ = nullptr; // Сбрасываем указатель, если инициализация не удалась
        return false;
    }

    // Резервируем память для временного буфера обмена только если он требуется.
    if (swap_output_on_forward_) {
        temp_swap_buffer_.resize(fft_size_);
    } else {
        // Если обмен не нужен, освобождаем память буфера
        temp_swap_buffer_.clear();
        temp_swap_buffer_.shrink_to_fit();
    }
    
    return true;
}

void FFT_Worker::Reset() {
    // Очистка векторов и освобождение их памяти
    p_fft_spec_mem_.clear();
    p_fft_spec_mem_.shrink_to_fit(); 

    temp_init_buffer_.clear();
    temp_init_buffer_.shrink_to_fit();

    fft_work_buf_.clear();
    fft_work_buf_.shrink_to_fit();

    temp_swap_buffer_.clear();
    temp_swap_buffer_.shrink_to_fit();
    
    fft_size_ = 0;
    fft_order_ = 0;
    swap_output_on_forward_ = false;
    pFFTSpec_ = nullptr; // Указатель теперь недействителен
}

bool FFT_Worker::ForwardFFT(const std::vector<Ipp32fc>& input_data, std::vector<Ipp32fc>& output_data) {
    // Попытка инициализации/переинициализации, если размеры или режим обмена изменились
    if (!Init(static_cast<int>(input_data.size()), swap_output_on_forward_)) {
        return false; 
    }

    // Убедимся, что выходной буфер имеет правильный размер
    if (output_data.size() < static_cast<size_t>(fft_size_)) {
        output_data.resize(fft_size_);
    }

    return ForwardFFT(input_data.data(), static_cast<int>(input_data.size()), output_data.data(), static_cast<int>(output_data.size()));
}

bool FFT_Worker::ForwardFFT(const Ipp32fc* input_data, int input_data_size, Ipp32fc* output_data, int output_data_size) {
    // Проверки на корректность состояния и размеров
    if (!pFFTSpec_ || fft_work_buf_.empty() || input_data_size < fft_size_ || output_data_size < fft_size_) {
        // FFT не инициализирован или размеры буферов недостаточны
        return false;
    }
    if (!input_data || !output_data) {
        throw std::invalid_argument("FFT_Worker::ForwardFFT: Input/output data pointers are null.");
    }

    IppStatus status;
    if (swap_output_on_forward_) {
        // Выполняем прямое БПФ во временный буфер
        status = ippsFFTFwd_CToC_32fc(input_data, temp_swap_buffer_.data(), pFFTSpec_, fft_work_buf_.data());
        if (status != ippStsNoErr) {
            return false;
        }
        // После БПФ обмениваем половины из временного буфера в выходной
        SwapHalfes(temp_swap_buffer_.data(), output_data, fft_size_);
    } else {
        // Если обмен не требуется, выполняем БПФ напрямую
        status = ippsFFTFwd_CToC_32fc(input_data, output_data, pFFTSpec_, fft_work_buf_.data());
    }
    
    return status == ippStsNoErr;
}

bool FFT_Worker::InverseFFT(const std::vector<Ipp32fc>& input_data, std::vector<Ipp32fc>& output_data) {
    // Попытка инициализации/переинициализации, если размеры или режим обмена изменились
    if (!Init(static_cast<int>(input_data.size()), swap_output_on_forward_)) {
        return false; 
    }

    // Убедимся, что выходной буфер имеет правильный размер
    if (output_data.size() < static_cast<size_t>(fft_size_)) {
        output_data.resize(fft_size_);
    }

    return InverseFFT(input_data.data(), static_cast<int>(input_data.size()), output_data.data(), static_cast<int>(output_data.size()));
}

bool FFT_Worker::InverseFFT(const Ipp32fc* input_data, int input_data_size, Ipp32fc* output_data, int output_data_size) {
    // Проверки на корректность состояния и размеров
    if (!pFFTSpec_ || fft_work_buf_.empty() || input_data_size < fft_size_ || output_data_size < fft_size_) {
        // FFT не инициализирован или размеры буферов недостаточны
        return false;
    }
    if (!input_data || !output_data) {
        throw std::invalid_argument("FFT_Worker::InverseFFT: Input/output data pointers are null.");
    }

    // Для обратного БПФ обмен половинами не требуется (согласно новому требованию).
    // IPP_FFT_DIV_INV_BY_N, указанный в Init, автоматически масштабирует результат.
    IppStatus status = ippsFFTInv_CToC_32fc(input_data, output_data, pFFTSpec_, fft_work_buf_.data());
    
    return status == ippStsNoErr;
}