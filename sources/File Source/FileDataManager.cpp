#include "FileDataManager.h"
#include "FileReadCore.h"
using namespace file_source;

//=================================== FileDataManager Implementation =============================

// �����������: �������������� ��������� �����
file_source::FileDataManager::FileDataManager(const file_source::file_params& params) : 
    params_(params)  // ������������� ����������� ������ �� ���������
{
}

// ������������� ��������� ��� ����������� ARK
void file_source::FileDataManager::InitReader(const fluctus::ArkWptr& reader, 
                                            const int64_t carrier, 
                                            const int64_t samplerate,  
                                            const int64_t block_size)
{
    auto& listener = listeners_.try_emplace(reader, reader, params_).first->second; // ������� ��� �������� ������������ listener
    listener.StopProcess();              // ������������� ������� ������� (���� ��������)
    listener.SetBaseParams(carrier, samplerate, block_size);  // ����������� ������� ���������
}

// ������ ������ ������ ������� ��� ���������� ARK
void file_source::FileDataManager::ReadAround(const fluctus::ArkWptr& reader, const double pos_ratio)
{
    auto& listener = listeners_.try_emplace(reader, reader, params_).first->second;  // �������� listener (������ ���� ���������������)
    listener.StartReadAround(pos_ratio);  // ����������� ������ ������
}

// �������� ��������� ��� ���������� ARK
void file_source::FileDataManager::DeleteReader(const fluctus::ArkWptr& reader)
{
    auto it = listeners_.find(reader);
    if(it == listeners_.end()) return;   // �����, ���� listener �� ������
    it->second.StopProcess();           // ��������� �������� ����� ���������
    listeners_.erase(it);                // �������� �� map
}

//=================================== FileDataListener Implementation ============================

// �����������: �������������� weak_ptr �� ARK � ��������� �����
file_source::FileDataListener::FileDataListener(const fluctus::ArkWptr& weak_ptr, const file_params& params):
    target_ark_(weak_ptr),  // ������������� ������ �� weak_ptr
    params_(params)         // ������������� ����������� ������ �� ���������
{
}

// ��������� ������� ���������� (�������, sample rate)
void file_source::FileDataListener::SetBaseParams(const int64_t carrier, 
                                                const int64_t samplerate, 
                                                const int64_t block_size)
{
    data_info_.freq_info_.carrier = carrier;      // ��������� ������� �������
    data_info_.freq_info_.samplerate = samplerate; // ��������� ������� �������������
    block_size_ = block_size;                     // ���������� ������� �����
}

// ��������� �������� ������������ ��������
void file_source::FileDataListener::StopProcess()
{
    if(process_anchor_.valid()) 
        process_anchor_.get();  // ���������, ���� ����� �� ����������
}

// ����������� ������ ������ ������ �������
bool file_source::FileDataListener::StartReadAround(const double pos_ratio)
{  
    // ������ ReadAroundProcess � ��������� ������
    process_anchor_ = std::async(std::launch::async, 
                                &FileDataListener::ReadAroundProcess, 
                                this, 
                                pos_ratio, 
                                block_size_);
    return true;
}

// �������� �������������� ������ � ARK
void file_source::FileDataListener::SendPreparedData()
{
    auto locked_ark = target_ark_.lock();  // ������� �������� shared_ptr �� weak_ptr
    if (!locked_ark)
        throw std::exception("ark doesn't exist - can not send data!");
    locked_ark->SendData(data_info_);  // �������� ������
}

// ��������� �������� ��������� ���������
const file_source::FileDataListener::ListenerState file_source::FileDataListener::GetState() const
{
    return state_;
}

// �������� ������� ������ ������ ������ �������
void file_source::FileDataListener::ReadAroundProcess(double pos_ratio, const int64_t out_data_size)
{
    FileReader reader;
    if(!reader.SetFileParams(params_))  // ��������� ��������� ��������
    {
        // ����� �������� callback ��� ������ (�� ����������� ������)
        return;
    }
    
    state_ = kReadAround;  // ��������� ��������� "� �������� ������"
    
    // ������ ������ ������ �������
    if(!reader.GetDataAround(pos_ratio, out_data_size, data_info_.data_vec)) 
    {
        state_ = kProcessStopped;  // ������ ������
        return;
    }
    
    std::vector<Ipp32fc> data_vec(out_data_size);

    ippsConvert_16s32f((Ipp16s*)data_info_.data_vec.data(), (Ipp32f*)data_vec.data(), out_data_size * 2);
    data_info_.data_vec.swap((std::vector<uint8_t>&)data_vec);


    SendPreparedData();   // �������� ������ - �������� ������
    state_ = kNoProcess;  // ����� ���������
}