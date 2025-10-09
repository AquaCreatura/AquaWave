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
    
    listener.WaitProcess();              // ������������� ������� ������� (���� ��������)
    listener.SetBaseParams(carrier, samplerate, block_size);  // ����������� ������� ���������
}

void file_source::FileDataManager::StartReading(const fluctus::ArkWptr & reader, const double start_pos, const double end_pos, const SortOfReading read_type)
{
	auto& listener = listeners_.try_emplace(reader, reader, params_).first->second;  // �������� listener (������ ���� ���������������)
	// ����������� ������ ������
	switch (read_type)
	{
	case file_source::FileDataManager::kReadAround: // ������ ������ ������ ������� 
		listener.StartReadAround(start_pos);  
		break;
	case file_source::FileDataManager::kReadChunksInRange:
		listener.StartReadChunksInRange(start_pos, end_pos);
		break;
	default:
		break;
	}
	
}



// �������� ��������� ��� ���������� ARK
void file_source::FileDataManager::DeleteReader(const fluctus::ArkWptr& reader)
{
    auto it = listeners_.find(reader);
    if(it == listeners_.end()) return;   // �����, ���� listener �� ������
    it->second.WaitProcess();           // ��������� �������� ����� ���������
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
void file_source::FileDataListener::WaitProcess()
{
    if(process_anchor_.valid()) 
        process_anchor_.get();  // ���������, ���� ����� �� ����������
}

// ����������� ������ ������ ������ �������
bool file_source::FileDataListener::StartReadAround(const double pos_ratio)
{  
    WaitProcess();
    // ������ ReadAroundProcess � ��������� ������
    process_anchor_ = std::async(std::launch::async, 
                                &FileDataListener::ReadAroundProcess, 
                                this, 
                                pos_ratio, 
                                block_size_);
    return true;
}

bool file_source::FileDataListener::StartReadChunksInRange(double start_pos, double end_pos)
{
	WaitProcess();
	// ������ ReadAroundProcess � ��������� ������
	process_anchor_ = std::async(std::launch::async,
		&FileDataListener::ReadChunksInRangeProcess,
		this,
		start_pos, end_pos);
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

void file_source::FileDataListener::ReadChunksInRangeProcess(double start_pos, double end_pos)
{
	const auto chunk_size = block_size_;
	StreamReader reader;
	if (!reader.SetFileParams(params_))  // ��������� ��������� ��������
	{
		return;
	}

	state_ = kReadChunksInRange;  // ��������� ��������� "� �������� ������"
	reader.InitFloat(start_pos, end_pos, chunk_size); //�������������� � ������������ ���������
	// ������ ������ ������ �������
	
	while (reader.ReadStream(data_info_.data_vec))
	{
		auto out_data_size = data_info_.data_vec.size() / 4;
		std::vector<Ipp32fc> data_vec(out_data_size);

		ippsConvert_16s32f((Ipp16s*)data_info_.data_vec.data(), (Ipp32f*)data_vec.data(), out_data_size * 2);
		data_info_.data_vec.swap((std::vector<uint8_t>&)data_vec);
		data_info_.time_point = 0.;

		SendPreparedData();   // �������� ������ - �������� ������
	}
	state_ = kNoProcess;  // ����� ���������
}

// �������� ������� ������ ������ ������ �������
void file_source::FileDataListener::ReadAroundProcess(double pos_ratio, const int64_t out_data_size)
{
	FileReader reader;
    if(!reader.SetFileParams(params_))  // ��������� ��������� ��������
    {
        return;
    }
    state_ = ListenerState::kReadAround;  // ��������� ��������� "� �������� ������"
	reader;
    // ������ ������ ������ �������
    if(!reader.GetDataAround(pos_ratio, out_data_size, data_info_.data_vec)) 
    {
        state_ = kProcessStopped;  // ������ ������
        return;
    }
    
    std::vector<Ipp32fc> data_vec(out_data_size);

    ippsConvert_16s32f((Ipp16s*)data_info_.data_vec.data(), (Ipp32f*)data_vec.data(), out_data_size * 2);
    data_info_.data_vec.swap((std::vector<uint8_t>&)data_vec);
    data_info_.time_point = pos_ratio;

    SendPreparedData();   // �������� ������ - �������� ������
    state_ = kNoProcess;  // ����� ���������
}