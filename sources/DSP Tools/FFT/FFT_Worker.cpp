#include "FFT_Worker.h"
#include <cmath>     // ��� std::log2
#include <stdexcept> // ��� ��������� ������

// ���������� ��������������� ������� SwapHalfes
// ---------------------------------------------

void SwapHalfes(const Ipp32fc* input_data, Ipp32fc* output_data, int data_size) {
    if (!input_data || !output_data || data_size <= 0) {
        throw std::invalid_argument("SwapHalfes: Invalid input parameters (null pointers or data_size <= 0).");
    }
    int half_size = data_size / 2;
    // �������� ������ �������� � ������ ��������� ������
    ippsCopy_32fc(input_data + half_size, output_data, half_size);
    // �������� ������ �������� �� ������ �������� ��������� ������
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
    // ���������� ��������� ����� ��� ����������� ������ �� �����
    std::vector<Ipp32fc> temp_buffer(half_size); 
    
    ippsCopy_32fc(input_data.data() + half_size, temp_buffer.data(), half_size);
    ippsCopy_32fc(input_data.data(), input_data.data() + half_size, half_size);
    ippsCopy_32fc(temp_buffer.data(), input_data.data(), half_size);
}

// ���������� ������� ������ FFT_Worker
// ------------------------------------

FFT_Worker::FFT_Worker() 
    : fft_size_(0), 
      fft_order_(0), 
      swap_output_on_forward_(false),
      pFFTSpec_(nullptr) {
    // ������� ���������������� �� ��������� �������
}

FFT_Worker::~FFT_Worker() {
    // ������� (std::vector<Ipp8u> � std::vector<Ipp32fc>) ������������� 
    // ����������� ���� ������ ��� ����������� ������� FFT_Worker.
    // ��� ������������� � ����� ������� ippsFree ��� delete.
}

bool FFT_Worker::Init(int initial_data_size, const bool swap_output_on_forward) {
    if (initial_data_size <= 0) {
        throw std::invalid_argument("FFT_Worker::Init: initial_data_size must be greater than 0.");
    }

    const int new_fft_size = GetPow2Closest(initial_data_size);
    const int new_fft_order = static_cast<int>(std::log2(new_fft_size));

    // ���� ������ ��� � ���� ������ �� ����������, ��� ������������� � ��������� �������������
    if (new_fft_size == fft_size_ && swap_output_on_forward == swap_output_on_forward_) {
        return true;
    }

    fft_size_ = new_fft_size;
    fft_order_ = new_fft_order;
    swap_output_on_forward_ = swap_output_on_forward;

    // ����� ������� ��� ��������� ���.
    // IPP_FFT_DIV_INV_BY_N ��������, ��� ippsFFTInv_CToC_32fc ����� ������ ��������� �� N.
    // ���� ��� ��������� ������ ������� (��� � ����� �������), ����������� ippAlgHintNone
    // � ��������� ������� �������������� ����� InverseFFT.
    auto fft_div_mode = IPP_FFT_DIV_BY_SQRTN; 
    IppHintAlgorithm alg_hint = ippAlgHintFast; // ���������� hintFast ��� ������������������

    int p_fft_spec_size = 0;
    int p_fft_spec_buf_size = 0;
    int fft_external_buf_size = 0;

    // �������� ������� �������, ����������� ��� IPP FFT
    IppStatus status = ippsFFTGetSize_C_32fc(
        fft_order_,         // ������� ��� (log2(N))
        fft_div_mode,
        alg_hint,
        &p_fft_spec_size,
        &p_fft_spec_buf_size,
        &fft_external_buf_size
    );

    if (status != ippStsNoErr) {
        // ��������, log2(initial_data_size) ������� ������� ��� ������ ������ IPP
        // � �������� ���� ����� �������� ����� ��������� ����������� ������� IPP.
        return false;
    }

    // �������� ������ ���������� ������� std::vector, ���� ��� ������ ����������.
    // resize() �����������, ��� ������� ����� �����������.
    if (fft_work_buf_.size() < static_cast<size_t>(fft_external_buf_size)) {
        fft_work_buf_.resize(fft_external_buf_size);
    }
    if (p_fft_spec_mem_.size() < static_cast<size_t>(p_fft_spec_size)) {
        p_fft_spec_mem_.resize(p_fft_spec_size);
    }
    if (temp_init_buffer_.size() < static_cast<size_t>(p_fft_spec_buf_size)) {
        temp_init_buffer_.resize(p_fft_spec_buf_size);
    }

    // �������������� ��������� ������������ FFT.
    // pFFTSpec_ ����� ��������� �� ������ p_fft_spec_mem_.
    pFFTSpec_ = reinterpret_cast<IppsFFTSpec_C_32fc*>(p_fft_spec_mem_.data());

    status = ippsFFTInit_C_32fc(
        &pFFTSpec_,          // ����� ��������� �� ������������
        fft_order_,          // ������� ��� (log2(N))
        fft_div_mode,
        alg_hint,
        p_fft_spec_mem_.data(), // ����� ��� ������������
        temp_init_buffer_.data() // ��������� ����� ��� �������������
    );

    if (status != ippStsNoErr) {
        pFFTSpec_ = nullptr; // ���������� ���������, ���� ������������� �� �������
        return false;
    }

    // ����������� ������ ��� ���������� ������ ������ ������ ���� �� ���������.
    if (swap_output_on_forward_) {
        temp_swap_buffer_.resize(fft_size_);
    } else {
        // ���� ����� �� �����, ����������� ������ ������
        temp_swap_buffer_.clear();
        temp_swap_buffer_.shrink_to_fit();
    }
    
    return true;
}

void FFT_Worker::Reset() {
    // ������� �������� � ������������ �� ������
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
    pFFTSpec_ = nullptr; // ��������� ������ ��������������
}

bool FFT_Worker::ForwardFFT(const std::vector<Ipp32fc>& input_data, std::vector<Ipp32fc>& output_data) {
    // ������� �������������/�����������������, ���� ������� ��� ����� ������ ����������
    if (!Init(static_cast<int>(input_data.size()), swap_output_on_forward_)) {
        return false; 
    }

    // ��������, ��� �������� ����� ����� ���������� ������
    if (output_data.size() < static_cast<size_t>(fft_size_)) {
        output_data.resize(fft_size_);
    }

    return ForwardFFT(input_data.data(), static_cast<int>(input_data.size()), output_data.data(), static_cast<int>(output_data.size()));
}

bool FFT_Worker::ForwardFFT(const Ipp32fc* input_data, int input_data_size, Ipp32fc* output_data, int output_data_size) {
    // �������� �� ������������ ��������� � ��������
    if (!pFFTSpec_ || fft_work_buf_.empty() || input_data_size < fft_size_ || output_data_size < fft_size_) {
        // FFT �� ��������������� ��� ������� ������� ������������
        return false;
    }
    if (!input_data || !output_data) {
        throw std::invalid_argument("FFT_Worker::ForwardFFT: Input/output data pointers are null.");
    }

    IppStatus status;
    if (swap_output_on_forward_) {
        // ��������� ������ ��� �� ��������� �����
        status = ippsFFTFwd_CToC_32fc(input_data, temp_swap_buffer_.data(), pFFTSpec_, fft_work_buf_.data());
        if (status != ippStsNoErr) {
            return false;
        }
        // ����� ��� ���������� �������� �� ���������� ������ � ��������
        SwapHalfes(temp_swap_buffer_.data(), output_data, fft_size_);
    } else {
        // ���� ����� �� ���������, ��������� ��� ��������
        status = ippsFFTFwd_CToC_32fc(input_data, output_data, pFFTSpec_, fft_work_buf_.data());
    }
    
    return status == ippStsNoErr;
}

bool FFT_Worker::InverseFFT(const std::vector<Ipp32fc>& input_data, std::vector<Ipp32fc>& output_data) {
    // ������� �������������/�����������������, ���� ������� ��� ����� ������ ����������
    if (!Init(static_cast<int>(input_data.size()), swap_output_on_forward_)) {
        return false; 
    }

    // ��������, ��� �������� ����� ����� ���������� ������
    if (output_data.size() < static_cast<size_t>(fft_size_)) {
        output_data.resize(fft_size_);
    }

    return InverseFFT(input_data.data(), static_cast<int>(input_data.size()), output_data.data(), static_cast<int>(output_data.size()));
}

bool FFT_Worker::InverseFFT(const Ipp32fc* input_data, int input_data_size, Ipp32fc* output_data, int output_data_size) {
    // �������� �� ������������ ��������� � ��������
    if (!pFFTSpec_ || fft_work_buf_.empty() || input_data_size < fft_size_ || output_data_size < fft_size_) {
        // FFT �� ��������������� ��� ������� ������� ������������
        return false;
    }
    if (!input_data || !output_data) {
        throw std::invalid_argument("FFT_Worker::InverseFFT: Input/output data pointers are null.");
    }

    // ��� ��������� ��� ����� ���������� �� ��������� (�������� ������ ����������).
    // IPP_FFT_DIV_INV_BY_N, ��������� � Init, ������������� ������������ ���������.
    IppStatus status = ippsFFTInv_CToC_32fc(input_data, output_data, pFFTSpec_, fft_work_buf_.data());
    
    return status == ippStsNoErr;
}