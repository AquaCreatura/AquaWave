#include "FileDataManager.h"
#include "FileReadCore.h"
using namespace file_source;

//=================================== FileDataManager Implementation =============================

// Конструктор: инициализирует параметры файла
file_source::FileDataManager::FileDataManager(const fluctus::SourceDescription& params) : 
    params_(params)  // Инициализация константной ссылки на параметры
{
}

// Инициализация слушателя для конкретного ARK
void file_source::FileDataManager::InitReader(const fluctus::ArkWptr& reader, 
                                            const int64_t carrier, 
                                            const int64_t samplerate,  
                                            const int64_t block_size)
{
    auto& listener = listeners_.try_emplace(reader, reader, params_).first->second; // Создает или получает существующий listener
    
    listener.WaitProcess();              // Останавливает текущий процесс (если работает)
    listener.SetBaseParams(carrier, samplerate, block_size);  // Настраивает базовые параметры
}

void file_source::FileDataManager::StartReading(const fluctus::ArkWptr & reader, const double start_pos, const double end_pos, const SortOfReading read_type)
{
	auto& listener = listeners_.try_emplace(reader, reader, params_).first->second;  // Получает listener (должен быть инициализирован)
	// Асинхронный запуск чтения
	switch (read_type)
	{
	case file_source::FileDataManager::kReadAround: // Запуск чтения вокруг позиции 
		listener.StartReadAround(start_pos);  
		break;
	case file_source::FileDataManager::kReadChunksInRange:
		listener.StartReadChunksInRange(start_pos, end_pos);
		break;
	default:
		break;
	}
	
}



// Удаление слушателя для указанного ARK
void file_source::FileDataManager::DeleteReader(const fluctus::ArkWptr& reader)
{
    auto it = listeners_.find(reader);
    if(it == listeners_.end()) return;   // Выход, если listener не найден
    it->second.WaitProcessLocked();           // Остановка процесса перед удалением
    listeners_.erase(it);                // Удаление из map
}

void file_source::FileDataManager::StopAllReaders()
{
	for (auto &listener_iter : listeners_) {
		listener_iter.second.WaitProcessLocked();
	};
}

//=================================== FileDataListener Implementation ============================

// Конструктор: инициализирует weak_ptr на ARK и параметры файла
file_source::FileDataListener::FileDataListener(const fluctus::ArkWptr& weak_ptr, const SourceDescription& params):
    target_ark_(weak_ptr),  // Инициализация ссылки на weak_ptr
    file_params_(params)         // Инициализация константной ссылки на параметры
{
}

// Установка базовых параметров (частота, sample rate)
void file_source::FileDataListener::SetBaseParams(const int64_t carrier, 
                                                const int64_t samplerate, 
                                                const int64_t block_size)
{
    data_info_.freq_info_.carrier_hz = carrier;      // Установка несущей частоты
    data_info_.freq_info_.samplerate_hz = samplerate; // Установка частоты дискретизации

	fluctus::freq_params file_params = { file_params_.carrier_hz, file_params_.samplerate_hz };
	resampler_.Init(file_params, data_info_.freq_info_, true);
    block_size_ = block_size;                     // Сохранение размера блока
}

// Остановка текущего асинхронного процесса
void file_source::FileDataListener::WaitProcess()
{
	state_ = kNeedStop;
    if(process_anchor_.valid()) 
        process_anchor_.get();  // Блокирует, пока поток не завершится
}

void file_source::FileDataListener::WaitProcessLocked()
{
	tbb::spin_mutex::scoped_lock scoped_locker(init_mutex_);
	WaitProcess();

}

// Асинхронный запуск чтения вокруг позиции
bool file_source::FileDataListener::StartReadAround(const double pos_ratio)
{  
	tbb::spin_mutex::scoped_lock scoped_locker(init_mutex_);
    WaitProcess();
	state_ = ListenerState::kReadAround;  // Установка состояния "в процессе чтения"
    // Запуск ReadAroundProcess в отдельном потоке
    process_anchor_ = std::async(std::launch::async, 
                                &FileDataListener::ReadAroundProcess, 
                                this, 
                                pos_ratio, 
                                block_size_);
    return true;
}

bool file_source::FileDataListener::StartReadChunksInRange(double start_pos, double end_pos)
{
	tbb::spin_mutex::scoped_lock scoped_locker(init_mutex_);
	WaitProcess();
	state_ = kReadChunksInRange;  // Установка состояния "в процессе чтения"
	// Запуск ReadAroundProcess в отдельном потоке
	process_anchor_ = std::async(std::launch::async,
		&FileDataListener::ReadChunksInRangeProcess,
		this,
		start_pos, end_pos);
	return true;
}

// Отправка подготовленных данных в ARK
void file_source::FileDataListener::SendPreparedData()
{
    auto locked_ark = target_ark_.lock();  // Попытка получить shared_ptr из weak_ptr
    if (!locked_ark)
        throw std::exception("ark doesn't exist - can not send data!");
    locked_ark->SendData(data_info_);  // Отправка данных
}

// Получение текущего состояния слушателя
const file_source::FileDataListener::ListenerState file_source::FileDataListener::GetState() const
{
    return state_;
}

void file_source::FileDataListener::ReadChunksInRangeProcess(double start_pos, double end_pos)
{
	const auto chunk_size = block_size_;
	StreamReader reader;
	if (!reader.SetFileParams(file_params_))  // Настройка файлового читателя
	{
		state_ = kProcessStopped;  // Ошибка чтения
		return;
	}
	data_info_.time_point = 0.;
	reader.InitStartEndRatio(start_pos, end_pos, chunk_size);
	const double supposed_samples = (end_pos - start_pos) * file_params_.count_of_samples /
												file_params_.samplerate_hz * data_info_.freq_info_.samplerate_hz;


	std::vector<Ipp32fc> casted_vec(chunk_size);
	std::vector<Ipp32fc> chunk_to_send(chunk_size);
	size_t chunk_pos = 0;
	int64_t samples_processed = 0;
	double time_point = 0;
	while (state_ != kNeedStop) {
		if (!reader.ReadStream(data_info_.data_vec, time_point)) break;

		ippsConvert_16s32f(reinterpret_cast<Ipp16s*>(data_info_.data_vec.data()),
			reinterpret_cast<Ipp32f*>(casted_vec.data()),
			chunk_size * 2);

		resampler_.ProcessBlock(casted_vec.data(), casted_vec.size());
		auto& processed_data = resampler_.GetProcessedData();
		size_t processed_idx = 0;
		while (processed_idx < processed_data.size()) {
			size_t remaining = chunk_size - chunk_pos;
			size_t to_copy = std::min(remaining, processed_data.size() - processed_idx);

			ippsCopy_32fc(processed_data.data() + processed_idx, chunk_to_send.data() + chunk_pos, to_copy);
			chunk_pos += to_copy;
			processed_idx += to_copy;
			
			samples_processed += to_copy;
			if (chunk_pos == chunk_size) {
				data_info_.time_point = (samples_processed - chunk_size / 2) / supposed_samples;

				data_info_.data_vec.swap(reinterpret_cast<std::vector<uint8_t>&>(chunk_to_send));
				SendPreparedData();
				chunk_to_send.resize(chunk_size);
				chunk_pos = 0;
			}
		}
	}
	state_ = kNoProcess;  // Сброс состояния
}

// Основной процесс чтения данных вокруг позиции
void file_source::FileDataListener::ReadAroundProcess(double pos_ratio, const int64_t out_data_size)
{
	FileReader reader;
    if(!reader.SetFileParams(file_params_))  // Настройка файлового читателя
    {
		state_ = kProcessStopped;  // Ошибка чтения
        return;
    }
    // Чтение данных вокруг позиции
    if(!reader.GetDataAround(pos_ratio, out_data_size, data_info_.data_vec) ||
		(data_info_.data_vec.size() != out_data_size * 4))
    {
        state_ = kProcessStopped;  // Ошибка чтения
        return;
    }
    
    std::vector<Ipp32fc> data_vec(out_data_size);
	
    ippsConvert_16s32f((Ipp16s*)data_info_.data_vec.data(), (Ipp32f*)data_vec.data(), out_data_size * 2);
    data_info_.data_vec.swap((std::vector<uint8_t>&)data_vec);
    data_info_.time_point = pos_ratio;

    SendPreparedData();   // Успешное чтение - отправка данных
    state_ = kNoProcess;  // Сброс состояния
}