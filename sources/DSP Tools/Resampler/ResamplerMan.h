#pragma once
#include <memory>
#include "ResamplersImpl/ResamperInterface.h"
#include "../basic/freq_shifter.h"
namespace aqua_resampler
{

// ��������� ProcessingParams �������� ��������� ��������� �������.
struct ProcessingParams 
{
    int64_t samplerate_hz; // ������� ������� ������������� ������� � ������.
    double  carrier_hz;    // ������� �������� ������� � ������.
};

// ����� ResamplerManager ��������� ��������� ����������� (��������� ������� �������������) �������.
class ResamplerManager 
{
public:
    // �����������: �������������� ������ ��������� �����������.
    ResamplerManager();

    // ����������: ����������� ��� ������������ �������.
    ~ResamplerManager();

    // ����� ������������� ����������.
    // ���������:
    //   base_params  - �������� ��������� ��������� (��������, ������� ������� �������������).
    //   target_params- ��������� �������� �������, ������� �������� ����� ���������� � �������� �����������.
    //   precise      - ����, ����������� �� ������������� ������� ������� ������������� �����������.
    // ���������� true, ���� ������������� ������ �������, ����� false.
    bool Init(const ProcessingParams base_params, ProcessingParams& target_params, bool precise);

    // ����� ��������� ����� ������� ������.
    // input_data - ��������� �� ������ ������� ����������� ������.
    // size       - ���������� ��������� �������� �������.
    // ���������� true, ���� ������ ���������� �������, ����� false.
    bool ProcessBlock(const Ipp32fc* input_data, size_t size);

    // ����� ��� ��������� ������������ ������.
    // ���������� ������ �� ������, ���������� ����������� ������ ����� �����������.
    std::vector<Ipp32fc>& GetProcessedData();

    // ����� ��� ������������ ���� ��������, ������������ ����������� � ����������� ��������.
    void FreeResources();

private:
    // ������, ���������� ������������ (����������������) ������.
    std::vector<Ipp32fc>                processed_data_;
    // ��������� �� ������ ���������� ���������� (��������, ��������������� ��� ������ ���������).
    std::unique_ptr<ResamplerInterface> resampler_;

    // ��������� ����������, ���������� ��������� ������� � ������ ���������.
    ResamplerSettings                   settings_;

    // �����, �������������� ����� ������
    aqua_dsp_tools::FrequencyShifter    freq_shifter_;
    std::vector<Ipp32fc>                shifted_data_;
};

}