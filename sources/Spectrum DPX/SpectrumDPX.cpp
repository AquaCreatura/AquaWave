#include "SpectrumDPX.h" // �������� ��������� ������ SpectrumDPX
#include <ippvm.h>
using namespace dpx_core; // ���������� ������������ ��� dpx_core

// �����������: �������������� ��������� ��� ��������� �������.
// parrent: ��������� �� ������������ QWidget.
dpx_core::SpectrumDPX::SpectrumDPX(QWidget * parrent)
{
    // ������ � ��������� ����� ��������� �� ������ DpxChart ��� ���������.
    dpx_drawer_ = std::make_shared<ChartDPX>(parrent);
}

// ���������� ������ ��� ��������� ������� � �����������.
// data_info: ��������� � �������� ������� � ����������� � �������.
bool SpectrumDPX::SendData(fluctus::DataInfo const & data_info)
{
    // ���� ������� ������ �����, �������.
    if(data_info.data_vec.empty()) return true;

    // ������ �� ���������� � ������� � ������� ����������� ������.
    auto &freq_info  = data_info.freq_info_;
    auto &passed_data = (std::vector<Ipp32fc>&)data_info.data_vec; // ���������� ����.
    
    // ����� ��� ���������� ���.
    std::vector<Ipp32fc> transformed_data(passed_data.size());
    // ��������� ������ ������� �������������� ����� (���).
    fft_worker_.EnableSwapOnForward(true); 
    if(!fft_worker_.ForwardFFT(passed_data, transformed_data))
        return false;

    // ����� ��� �������� ������� (��������).
    std::vector<Ipp32f> power_vec(transformed_data.size());
    
    // ��������� ��������� (���������) ����������� �����.
    ippsPowerSpectr_32fc(transformed_data.data(), power_vec.data(), passed_data.size());
    ippsLog10_32f_A11(power_vec.data(), power_vec.data(), power_vec.size());
    ippsMulC_32f_I(10, power_vec.data(), power_vec.size());
    
    // ���������� ������� ���������� ��������� ��� �����������.
    Limits<double> freq_bounds = {freq_info.carrier - freq_info.samplerate / 2.,
                                   freq_info.carrier + freq_info.samplerate / 2.};
    
    // ���������� ����������� ��������� � ��������� ������� � ����������.
    dpx_drawer_->PushData(power_vec, freq_bounds);
    
    return true; // �����.
}

// ������������ ��������� "Dove".
// sent_dove: ����� ��������� �� ��������� Dove.
bool dpx_core::SpectrumDPX::SendDove(fluctus::DoveSptr const & sent_dove)
{
    // ���� ��������� ���������������, ����������� ����������.
    if (!sent_dove) throw std::invalid_argument("Not created message sent!");
    
    // �������� ������� �������� � "�����" �� ���������.
    auto target_val = sent_dove->target_ark;
    auto base_thought = sent_dove->base_thought;
    
    // ���� "�����" - "������ �� ������", �������.
    if      (base_thought == fluctus::DoveParrent::DoveThought::kNothing)
        return true;
    // ���� "�����" - ������ �� ������.
    else if (base_thought & fluctus::DoveParrent::DoveThought::kGetDialog)
    {
        // ����������� ���������� ������� � ������� ���������.
        sent_dove->show_widget = dpx_drawer_;
        return true; // ������ ���������.
    }
    // ������� ��������� �������� ������ ��� ���������� ���������.
    return ArkBase::SendDove(sent_dove);
}