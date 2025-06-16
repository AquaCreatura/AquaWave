#pragma once

#include <vector>
#include <ipps.h> // ��� Ipp32fc, IppStatus � ������ IPP-�����

// ��������������� ������� ��� ��������� ��������� ������� ������
// (������ ��� inline ��� � ��������� ����������� �����)
inline int GetPow2Closest(int n) {
    if (n <= 0) return 0;
    int power = 1;
    while (power < n) {
        power <<= 1;
    }
    return power;
}

// ��������������� ������� ��� ������ ���������� ������
// ��������� ������ ����� ������� ��� ��� ������������� �������
void SwapHalfes(const Ipp32fc* input_data, Ipp32fc* output_data, int data_size);
void SwapHalfes(const std::vector<Ipp32fc>& input_data, std::vector<Ipp32fc>& output_data);
void SwapHalfes_I(std::vector<Ipp32fc>& input_data); // In-place swap

class FFT_Worker {
public:
    // ����������� �� ���������
    FFT_Worker();

    // ���������� (������� ������������� ��������� �������)
    ~FFT_Worker();

    // ������������� FFT_Worker.
    // initial_data_size: �������������� ������ ������ ��� ���.
    //      ����� ������ ��������� ������� ������ ��� ����� �������.
    // swap_output_on_forward: ���� true, ����������� ����� ���������� ����� ������� ���.
    bool Init(int initial_data_size, const bool swap_output_on_forward = true);

    // ����� ��������� FFT_Worker, ������� ������� � ������������ ������.
    void Reset();

    // ���������� ������� ���.
    // ���� ������ ������� ������ �� ������������� ������������������� ������� ���,
    // ���������� ��������� �������������.
    bool ForwardFFT(const std::vector<Ipp32fc>& input_data, std::vector<Ipp32fc>& output_data);
    // ���������� ������� ��� ��� C-��������.
    bool ForwardFFT(const Ipp32fc* input_data, int input_data_size, Ipp32fc* output_data, int output_data_size);

    // ���������� ��������� ���.
    // ���� ������ ������� ������ �� ������������� ������������������� ������� ���,
    // ���������� ��������� �������������.
    bool InverseFFT(const std::vector<Ipp32fc>& input_data, std::vector<Ipp32fc>& output_data);
    // ���������� ��������� ��� ��� C-��������.
    bool InverseFFT(const Ipp32fc* input_data, int input_data_size, Ipp32fc* output_data, int output_data_size);

private:
    int fft_size_;                      // ������ ��� (��������� ������� 2)
    int fft_order_;                     // ������� ��� (log2(fft_size_))
    bool swap_output_on_forward_;       // ����: ��������� �� ����� ���������� ����� ������� ���

    // ��������� �� ��������� ������������ FFT.
    // ��������� �� ������ ������ p_fft_spec_mem_.
    IppsFFTSpec_C_32fc* pFFTSpec_;      
    
    // ������, ����������� std::vector ��� ��������������� ���������/������������
    // � ������������� ���������� ��������.
    std::vector<Ipp8u> p_fft_spec_mem_;     // ������ ��� ��������� ������������ FFT
    std::vector<Ipp8u> temp_init_buffer_;   // ��������� ����� ��� ippsFFTInit_C_32fc
    std::vector<Ipp8u> fft_work_buf_;       // ������� ����� ��� ippsFFTForw/Inv_CToC_32fc
    
    // ��������� ������ ��� �������� ������ ����������.
    // ���������� ���� ��� ��� ������������� ��� ����������� ���������� ���������.
    std::vector<Ipp32fc> temp_swap_buffer_; 
};