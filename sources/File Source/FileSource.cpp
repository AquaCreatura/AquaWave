#include "FileSource.h"

using namespace file_source;
using namespace fluctus;
// ========================================== FileSourceArk =================================

// �����������: ������� ������ � �������������� �������� ����������
file_source::FileSourceArk::FileSourceArk():
    listener_man_(file_info_)  // ������������� ��������� � ����������� �����
{
    dialog_ = std::make_shared<FileSourceDialog>();  // �������� ����������� ����
    UpdateSource();
    connect(dialog_.get(), &FileSourceDialog::UpdateSourceNeed, this, &FileSourceArk::UpdateSource);
   
}

file_source::FileSourceArk::~FileSourceArk()
{
}

// ���������� ��������� (Dove - "������" ��� ���������)
bool file_source::FileSourceArk::SendDove(fluctus::DoveSptr const& sent_dove)
{
    if (!sent_dove)
        throw std::invalid_argument("empty dove sent!");
    const auto &target_ark = sent_dove->target_ark ? sent_dove->target_ark: sent_dove->sender;
    const auto parrent_type = sent_dove->base_thought;
    
    // ��������� ������� ������
    if (parrent_type & fluctus::DoveParrent::kTieFront)
    {
        fluctus::DoveSptr message   = std::make_shared<fluctus::DoveParrent>();
        message->base_thought       = DoveParrent::kReset;
        target_ark->SendDove(message);
        return ArkBase::SendDove(sent_dove);
    }
    if (parrent_type & fluctus::DoveParrent::kUntieFront)
    {
        return ArkBase::SendDove(sent_dove);
    }
    
    // ������ ����������� ����
    if (parrent_type & fluctus::DoveParrent::kGetDialog)
    {
        sent_dove->show_widget = dialog_;  // ���������� ��������� �� ������
    }
    

    // ��������� ������������������ ������ ��� ��������� ���������
    if (parrent_type & fluctus::DoveParrent::kSpecialThought)
    {
        auto file_src_dove = std::dynamic_pointer_cast<FileSrcDove>(sent_dove);
        if (!file_src_dove)
            throw std::invalid_argument("wrong thought type!");
            
        const auto file_src_thought = file_src_dove->special_thought;
        
        // ������������� ��������
        if (file_src_thought & FileSrcDove::FileSrcDoveThought::kInitReaderInfo)
        {
            const auto carrier_hz    = file_src_dove->carrier_hz    ? *file_src_dove->carrier_hz    : file_info_.carrier_hz_;
            const auto samplerate_hz = file_src_dove->samplerate_hz ? *file_src_dove->samplerate_hz : file_info_.samplerate_hz_;
            listener_man_.InitReader(target_ark, carrier_hz, samplerate_hz, *file_src_dove->data_size);
        }
        
        // ������ ������ ������ �����
        if (file_src_thought & FileSrcDove::FileSrcDoveThought::kAskSingleDataAround)
        {
            listener_man_.ReadAround(target_ark, *file_src_dove->time_point_start);
        }
        
        // ������ ����������� ������ (�� �����������)
        if (file_src_thought & FileSrcDove::FileSrcDoveThought::kAskCyclicData)
        {
            //Do smth
        }
        
        // ������ ������ � ��������� (�� �����������)
        if (file_src_thought & FileSrcDove::FileSrcDoveThought::kAskSingleDataInRange)
        {
            //Do smth
        }
        if (file_src_thought & FileSrcDove::FileSrcDoveThought::kSetFileName)
        {
            const QString file_name = (*file_src_dove->file_info).file_name_;
            dialog_->SetFileName(file_name);
            //Do smth 
        }//kSetFile
        if (file_src_thought & FileSrcDove::FileSrcDoveThought::kGetFileInfo)
        {
            file_src_dove->file_info = this->file_info_;
            //Do smth
        }
    }
    return ArkBase::SendDove(sent_dove);
}

fluctus::ArkType file_source::FileSourceArk::GetArkType() const
{
    return fluctus::ArkType::kFileSource;
}

void file_source::FileSourceArk::UpdateSource()
{
    this->file_info_ = dialog_->GetFileInfo(); //Update info, according ui
    //Reset out arks
    {
        auto out_fleet   = GetFrontArks();
        fluctus::DoveSptr message   = std::make_shared<fluctus::DoveParrent>();
        message->base_thought       = DoveParrent::kReset;
        for(auto &out_ark: out_fleet) out_ark->SendDove(message);
    }
}

// �������� ������ (�� �����������)
bool file_source::FileSourceArk::SendData(fluctus::DataInfo const& data_info)
{
    return false;
}
