#pragma once
#ifndef BASIC_DSP_TOOLS
#define BASIC_DSP_TOOLS
#include <ipps.h>
namespace aqua_dsp_tools
{

// ����� FrequencyShifter ������������ ����� ������� ����������� ������ �� �������� �������.
// ������������� ������������ �� ������ ������� ������� � ������� �������.
// �����, ����� ����� �������� ��������� ����������� ���� ��� ����������� ������������� �������.
class FrequencyShifter 
{
public:
    // �����������.
    // baseCarrierHz  - ������� ������� (� ��) �������� �������.
    // targetCarrierHz- ������� (� ��), ������� ���������� �������� �� ������.
    // sampleRateHz   - ������� ������������� (� ��).
    FrequencyShifter();
    void Init       (double baseCarrierHz, double targetCarrierHz, double sampleRateHz);
    // ����������.
    ~FrequencyShifter();

    // ������������ ���� ������ � ���������� ��������� � ��������� �����.
    // inputData  - ��������� �� ������� ������ ����������� ������.
    // outputData - ��������� �� �������� ������, ���� ����� �������� ��������� ������.
    // blockSize  - ���������� ��������� � �����.
    // ���������� true, ���� ��������� ������ �������.
    bool ProcessBlock(const Ipp32fc* inputData, Ipp32fc* outputData, int blockSize);

    // ������������ ���� ������ "�� �����" (in-place): ��������� ������ ������������ ������ ��������.
    // data      - ��������� �� ������ ����������� ������, ������� ����� �������������.
    // blockSize - ���������� ��������� � �����.
    // ���������� true, ���� ��������� ������ �������.
    bool ProcessBlockInPlace(Ipp32fc* data, int blockSize);

private:
    Ipp32f phase_;    // ������� ���� ���������� ���� ��� ����������� ������������� ����� �������.
    Ipp32f tone_freq_; // ������� ���� ��� ������ � �������� �� ������: ����������� �� ������� ������� � ������� �������.
    Ipp32f tone_magn_; // ��������� ����, ������ ����� 1.0.
    std::vector<Ipp32fc> tone_vec_; //��������������� ���� ����������� ���������
};










}
#endif // !BASIC_DSP_TOOLS



