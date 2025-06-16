#ifndef RESAMPLER_TOOLS
#define RESAMPLER_TOOLS
#pragma once
#include <iostream>
#include <cmath>
#include <limits>

#include <utility>
#include <exception>

#include <vector>
#include <cstdint>
#include <ipps.h> // ��������������, ��� ����� ��������� ���������� ������� IPP


namespace aqua_resampler
{

// ������� ��� ������ �����, ����������� ������������ � ��������� ���� �����.
// ���������:
//   a            - ��������� ��������� ��������� (����� �����)
//   b            - ����������� ��������� ��������� (����� �����)
//   max_denom    - ������������ ���������� �������� ����������� ������� ����� (�� ��������� 100)
//   not_less_than- ��������� ����: ���� true, �������� ����� �� ����� ���� ������ (a / b)
// ���������� ���� <p, q>, �������������� ����� p/q, ����������� ������������ � �������� (a / b).
static std::pair<int, int> FindBestFraction(const double target, const int max_denom = 100, const bool not_less_than = true) 
{   
    // �������������� ���������� ��� �������� ��������� ��������� ����� � � ������.
    // best_num � best_denom - ��������� � ����������� ������ ��������� �����.
    int best_num = 0, best_denom = 1;
    // ���������� ������������� ������ ��� ����������� ��������� ��������, ����� ����� �������� ����� ����� ������� ������.
    double best_error = std::numeric_limits<double>::max();

    // ���������� ��� ���������� �������� ����������� �� 1 �� max_denom.
    for (int cur_denom = 1; cur_denom <= max_denom; ++cur_denom) {
        int cur_num;
        // ��� ������� ����������� ��������� ���������� ���������.
        // ���� ���������� ���� not_less_than, �������� ��������� ����� �������, ����� ����� cur_num/cur_denom �� ���� ������ �������� ���������.
        if (not_less_than) 
        {
            // ���������� ���������� �����: ceil(target * cur_denom).
            cur_num = static_cast<int>(std::ceil(target * cur_denom));
        } else 
        {
            // ����� �������� ���������, �������� �� ���������� ������: round(target * cur_denom).
            cur_num = static_cast<int>(std::round(target * cur_denom));
        }

        // ��������� �������� ����� ��� �������� cur_num � cur_denom.
        double fraction = static_cast<double>(cur_num) / cur_denom;
        // ���� ���������� ���� not_less_than � ����� ����������� ������ ��������, ���������� �.
        if (not_less_than && fraction < target) 
        {
            continue;
        }
        
        // ��������� ���������� ������� ����� ���������� ������ � ������� ���������.
        const double error = std::fabs(fraction - target);
        
        // ���� ������� ������ ������, ��� ����� ���������������, ��������� ������ �������.
        if (error < best_error) 
        {
            best_error = error;
            best_num = cur_num;
            best_denom = cur_denom;
        }
    }    
    // ���������� ��������� ����� � ���� ����: ��������� � �����������.
    return std::make_pair(best_num, best_denom);
}


// ������� GenerateWindowKoeffs ���������� ������������ ��� ���� ������� ������ ������.
// ���������:
//   cutoff_freq    - ������� ����� ������� (������ ���� � ��������� (0, 0.5))
//   coefs_num      - ���������� ������������� (������ ���� > 8 � <= 1'000'000)
//   window_type    - ��� ����, ������������� ��� ��������� ������������� (��������, Hamming, Blackman � �.�.)
//   coefs_complex  - ������, ���� ����� �������� ��������������� ����������� ������������
// ���������� true, ���� ������������ ������������� �������, ����� false.
static bool GenerateWindowKoeffs( const double          cutoff_freq,
                           const int             coefs_num,
                           const IppWinType      window_type,
                           std::vector<Ipp32fc>& coefs_complex )
{    
    // �������� ������������ ������� ����������.
    // ������� ����� ������ ���������� � ��������� (0, 0.5)
    // ���������� ������������� ������ ���� ������ 8 � �� ��������� 1'000'000.
    if( cutoff_freq <= 0.0 || cutoff_freq >= 0.5 || coefs_num <= 8 || coefs_num > 1000000 )
    {
        //std::cerr << "�������� ������� ���������: cutoff_freq = " << cutoff_freq << ", coefs_num = " << coefs_num << std::endl;
        return false;
    }

    // ����������� ������������ ������� ������ ��� ��������� �������������
    int coefs_gen_buf_size = 0;
    IppStatus status = ippsFIRGenGetBufferSize(coefs_num, &coefs_gen_buf_size);
    if(status != ippStsNoErr)
    {
        //std::cerr << "������ ��� ��������� ������� ������ ��� ��������� �������������, ������ = " << status << std::endl;
        return false;
    }

    // �������� ����� ��� ��������� �������������, ��������� ������ ����.
    std::vector<uint8_t> coefs_gen_buf(coefs_gen_buf_size);

    // ������� ������� ��� �������� ������������� � ������� double � float.
    std::vector<Ipp64f> coefs_real_double(coefs_num);
    std::vector<Ipp32f> coefs_real_float(coefs_num);

    // ����������� ����� ��� ����������� �������������.
    coefs_complex.resize(coefs_num);

    // ��������� ������������� ��������������� �������.
    // ippTrue ��������� �� ��, ��� ������������ ������ ���� �������������.
    status = ippsFIRGenLowpass_64f(cutoff_freq, coefs_real_double.data(), coefs_num,
                                   window_type, ippTrue, coefs_gen_buf.data());
    if(status != ippStsNoErr)
    {
        //std::cerr << "������ ��� ��������� ������������� ��������������� �������, ������ = " << status << std::endl;
        return false;
    }

    // �������������� ������������� �� double � float.
    status = ippsConvert_64f32f(coefs_real_double.data(), coefs_real_float.data(), coefs_num);
    if(status != ippStsNoErr)
    {
        //std::cerr << "������ ��� ����������� ������������� �� double � float, ������ = " << status << std::endl;
        return false;
    }


    // �������������� ������������ ������������� � ����������� ������.
    // ������ �������� (nullptr) ���������, ��� ������ ����� ������������� ����� ����.
    status = ippsRealToCplx_32f(coefs_real_float.data(), nullptr, coefs_complex.data(), coefs_num);
    if(status != ippStsNoErr)
    {
        //std::cerr << "������ ��� �������������� ������������ ������������� � ����������� ������, ������ = " << status << std::endl;
        return false;
    }

    // ���� ��� ����� ��������� �������, ���������� true.
    return true;
}






}
#endif // RESAMPLER_TOOLS
