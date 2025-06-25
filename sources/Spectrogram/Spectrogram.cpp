#include "Spectrogram.h"
#include <ippvm.h>
using namespace spg_core; // ���������� ������������ ��� dpx_core

// �����������: �������������� ��������� ��� ��������� �������.
// parrent: ��������� �� ������������ QWidget.
Spectrogram::Spectrogram(QWidget * parrent) : 
    spg_drawer_(new spg_core::ChartSPG()),
    requester_(spg_drawer_->GetSpectrogramInfo(), time_bounds_) 
{
    window_ = std::make_shared<SpgWindow>();
    window_->SetChartWindow(spg_drawer_);
    connect(spg_drawer_, &ChartSPG::NeedRequest, &requester_, &SpgRequester::RequestData/*, Qt::QueuedConnection*/);
}

spg_core::Spectrogram::~Spectrogram()
{
}

// ���������� ������ ��� ��������� ������� � �����������.
// data_info: ��������� � �������� ������� � ����������� � �������.
bool Spectrogram::SendData(fluctus::DataInfo const & data_info)
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
    draw_data draw_data;
    draw_data.freq_bounds = freq_bounds;
    draw_data.time_pos    = data_info.time_point;
    draw_data.data        = power_vec;
    spg_drawer_->PushData(draw_data);
    
    return true; // �����.
}

// ������������ ��������� "Dove".
// sent_dove: ����� ��������� �� ��������� Dove.
bool Spectrogram::SendDove(fluctus::DoveSptr const & sent_dove)
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
        sent_dove->show_widget = window_;
        return true; // ������ ���������.
    }
    // ������� ��������� �������� ������ ��� ���������� ���������.
    if(base_thought & DoveParrent::DoveThought::kTieBehind)
    {
        if(target_val->GetArkType() != ArkType::kFileSource) throw std::logic_error("Only signal sources are able to connect!");
        requester_.Initialise(target_val, this->shared_from_this());
    }
    return ArkBase::SendDove(sent_dove);
}

ArkType spg_core::Spectrogram::GetArkType() const
{
    return ArkType::kFileSpectrogram;
}
