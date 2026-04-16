#include "FileDataManager.h"
#include <qdebug.h>
using namespace file_source;

//=================================== FileDataManager Implementation =============================

// Конструктор: инициализирует параметры файла
file_source::FileDataManager::FileDataManager(const fluctus::SourceDescription& params) : 
    params_(params)  // Инициализация константной ссылки на параметры
{
}

// Инициализация слушателя для конкретного ARK
void file_source::FileDataManager::InitReader(const fluctus::ArkWptr& reader, InitParams &setup)
{
	auto& listener = listeners_.try_emplace(reader, reader, params_).first->second; // Создает или получает существующий listener
    listener.WaitProcess();             // Останавливает текущий процесс (если работает)
    listener.SetBaseParams(setup);		// Настраивает базовые параметры
}

void file_source::FileDataManager::UpdateChunkSize(const fluctus::ArkWptr & reader, const int chunk_size)
{
	auto& listener = listeners_.find(reader)->second;
	listener.SetChunkSize(chunk_size);
}

void file_source::FileDataManager::StartReading(const fluctus::ArkWptr & reader, Limits<double> time_bounds, const FileSrcDove::FileSrcDoveThought read_type)
{
	auto& listener = listeners_.try_emplace(reader, reader, params_).first->second;  // Получает listener (должен быть инициализирован)
	// Асинхронный запуск чтения
	switch (read_type)
	{
	case FileSrcDove::FileSrcDoveThought::kAskChunkAround: // Запуск чтения вокруг позиции 
		listener.StartSingleAround(time_bounds.low);
		break;
	case FileSrcDove::FileSrcDoveThought::kAskChunksInRange:
		listener.StartSingleInRange(time_bounds);
		break;
	case FileSrcDove::FileSrcDoveThought::kAskLoopInRange:
		listener.StartLoopInRange(time_bounds);
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
void file_source::FileDataListener::SetBaseParams(InitParams &setup)
{
	tbb::spin_mutex::scoped_lock scoped_locker(init_mutex_);
    data_info_.freq_info_.carrier_hz = setup.carrier_hz;      // Установка несущей частоты
    data_info_.freq_info_.samplerate_hz = setup.samplerate_hz; // Установка частоты дискретизации

	resampler_.SetBaseParams(file_params_.carrier_hz, file_params_.samplerate_hz);
	resampler_.SetTargetParams(setup.carrier_hz, setup.samplerate_hz, setup.banwidth_hz);
    block_size_ = setup.chunk_size;                     // Сохранение размера блока
}

void file_source::FileDataListener::SetChunkSize(const int chunk_size)
{
	block_size_ = chunk_size;
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
bool file_source::FileDataListener::StartSingleAround(const double pos_ratio)
{  
	tbb::spin_mutex::scoped_lock scoped_locker(init_mutex_);
    WaitProcess();
	state_ = ListenerState::kReadAround;  


	

    process_anchor_ = std::async(std::launch::async, 
                                &FileDataListener::ReadAroundProcess, 
                                this, 
                                pos_ratio, 
                                block_size_);


    return true;
}

bool file_source::FileDataListener::StartSingleInRange(const Limits<double>& time_bounds)
{
	tbb::spin_mutex::scoped_lock scoped_locker(init_mutex_);
	WaitProcess();
	state_ = kSingleReadInRange; 
	process_anchor_ = std::async(std::launch::async, &FileDataListener::ReadChunksInRangeProcess, this, time_bounds);
	return true;
}

bool file_source::FileDataListener::StartLoopInRange(const Limits<double>& time_bounds)
{
	tbb::spin_mutex::scoped_lock scoped_locker(init_mutex_);
	WaitProcess();
	state_ = kLoopReadInRange;  
	process_anchor_ = std::async(std::launch::async, &FileDataListener::LoopReadInRangeProcess, this, time_bounds);
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

void file_source::FileDataListener::ReadChunksInRangeProcess(const Limits<double>& time_bounds)
{
	const auto chunk_size = block_size_;
	StreamReader reader;
	if (!reader.SetFileParams(file_params_))  // Настройка файлового читателя
	{
		state_ = kProcessStopped;  // Ошибка чтения
		return;
	}
	data_info_.time_point = 0.;
	reader.InitStartEndRatio(time_bounds);
	const double supposed_samples = (time_bounds.delta()) * file_params_.count_of_samples /
												file_params_.samplerate_hz * data_info_.freq_info_.samplerate_hz;


	std::vector<Ipp32fc> casted_vec(chunk_size);
	std::vector<Ipp32fc> chunk_to_send(chunk_size);
	std::vector<uint8_t> read_data;
	size_t chunk_pos = 0;
	int64_t samples_processed = 0;
	while (state_ != kNeedStop) {
		if (!reader.ReadStream(read_data, block_size_))
			break;
		if (read_data.size() != 1 << int(log2(read_data.size())))
			break;
		ippsConvert_16s32f(reinterpret_cast<Ipp16s*>(read_data.data()),
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

void file_source::FileDataListener::LoopReadInRangeProcess(const Limits<double>& time_bounds)
{
	// Проверка на пустой диапазон
	if (time_bounds.delta() <= 0)
	{
		state_ = kProcessStopped;
		return;
	}
	StreamReader reader;
	// Однократная настройка файлового читателя
	if (!reader.SetFileParams(file_params_))
	{
		state_ = kProcessStopped;
		return;
	}
	// Ожидаемое количество выходных отсчётов за один проход диапазона
	const double supposed_samples = (time_bounds.delta()) *
		file_params_.count_of_samples /
		file_params_.samplerate_hz *
		data_info_.freq_info_.samplerate_hz;
	
	auto chunk_size = block_size_;	
	reader.InitStartEndRatio(time_bounds);
	std::vector<Ipp32fc> casted_vec(chunk_size);
	std::vector<Ipp32fc> chunk_to_send(chunk_size);
	std::vector<uint8_t> read_data;
	size_t chunk_pos = 0;               // текущая позиция в выходном чанке
	int64_t samples_processed = 0;      // общее число обработанных выходных отсчётов

	while (state_ != kNeedStop)
	{
		if (block_size_ != chunk_size) {
			chunk_size = block_size_;
			casted_vec.resize(chunk_size);
			chunk_to_send.resize(chunk_size);

			chunk_pos = 0;               // текущая позиция в выходном чанке
			samples_processed = 0;      // общее число обработанных выходных отсчётов

			reader.InitStartEndRatio(time_bounds);
		}
		// Пытаемся прочитать очередной блок из файла
		if (!reader.ReadStream(read_data, chunk_size))
		{
			// Достигнут конец диапазона — переинициализируем читатель для нового прохода
			reader.InitStartEndRatio(time_bounds);
			// Пробуем прочитать снова; если опять неудача — фатальная ошибка
			if (!reader.ReadStream(read_data, chunk_size))
			{
				state_ = kProcessStopped;
				return;
			}
		}

		// Преобразование int16 -> float (комплексные отсчёты)
		switch (file_params_.data_type_)
		{
		case IppDataType::ipp16sc: {
			ippsConvert_16s32f((Ipp16s*)(read_data.data()), (Ipp32f*)(casted_vec.data()), chunk_size * 2);
			break;
		}
		default:
			casted_vec.swap((std::vector<Ipp32fc>&)read_data);
			break;
		}
		

		// Ресемплинг блока
		resampler_.ProcessBlock(casted_vec.data(), casted_vec.size());
		const auto& processed_data = resampler_.GetProcessedData();

		// Копируем обработанные отсчёты в выходной чанк
		size_t processed_idx = 0;
		while (processed_idx < processed_data.size())
		{
			size_t remaining = chunk_size - chunk_pos;
			size_t to_copy = std::min(remaining, processed_data.size() - processed_idx);

			ippsCopy_32fc(processed_data.data() + processed_idx,
				chunk_to_send.data() + chunk_pos,
				to_copy);

			chunk_pos += to_copy;
			processed_idx += to_copy;
			samples_processed += to_copy;

			// Если выходной чанк заполнен — отправляем
			if (chunk_pos == chunk_size)
			{
				// Вычисляем временную метку для отправляемого блока
				data_info_.time_point = (samples_processed - chunk_size / 2) / supposed_samples;

				data_info_.data_vec.swap((std::vector<uint8_t>&)(chunk_to_send));
				SendPreparedData();

				// Подготавливаем новый чанк
				chunk_to_send.resize(chunk_size);
				chunk_pos = 0;
			}
		}
	}

	state_ = kNoProcess;
}

// Основной процесс чтения данных вокруг позиции
void file_source::FileDataListener::ReadAroundProcess(double pos_ratio, const int64_t chunk_size)
{

    if(!file_reader_.SetFileParams(file_params_))  // Настройка файлового читателя
    {
		state_ = kProcessStopped;  // Ошибка чтения
        return;
    }
    // Чтение данных вокруг позиции
    if(!file_reader_.GetDataAround(pos_ratio, chunk_size, data_info_.data_vec) ||
		(data_info_.data_vec.size() != chunk_size * GetSampleSize(file_params_.data_type_) ))
    {
        state_ = kProcessStopped;  // Ошибка чтения
        return;
    }

	std::vector<Ipp32fc> casted_vec(chunk_size);
	switch (file_params_.data_type_)
	{
	case IppDataType::ipp16sc: {
		ippsConvert_16s32f((Ipp16s*)(data_info_.data_vec.data()), (Ipp32f*)(casted_vec.data()), chunk_size * 2);
		break;
	}
	default:
		casted_vec.swap((std::vector<Ipp32fc>&)data_info_.data_vec);
		break;
	}
    data_info_.data_vec.swap((std::vector<uint8_t>&)casted_vec);
    data_info_.time_point = pos_ratio;

    SendPreparedData();   // Успешное чтение - отправка данных
    state_ = kNoProcess;  // Сброс состояния
}

void file_source::FileDataListener::LoopReadPointsProcess()
{
	if (!file_reader_.SetFileParams(file_params_))  // Настройка файлового читателя
	{
		state_ = kProcessStopped;  // Ошибка чтения
		return;
	}
	int chunk_size = block_size_;
	while (state_ != kNeedStop)
	{
		chunk_size = block_size_;
		double pos_ratio = 0.;
		// Чтение данных вокруг позиции
		if (!file_reader_.GetDataAround(pos_ratio, chunk_size, data_info_.data_vec) ||
			(data_info_.data_vec.size() != chunk_size * GetSampleSize(file_params_.data_type_)))
		{
			state_ = kProcessStopped;  // Ошибка чтения
			return;
		}

		std::vector<Ipp32fc> casted_vec(chunk_size);
		switch (file_params_.data_type_)
		{
		case IppDataType::ipp16sc: {
			ippsConvert_16s32f((Ipp16s*)(data_info_.data_vec.data()), (Ipp32f*)(casted_vec.data()), chunk_size * 2);
			break;
		}
		default:
			casted_vec.swap((std::vector<Ipp32fc>&)data_info_.data_vec);
			break;
		}
		data_info_.data_vec.swap((std::vector<uint8_t>&)casted_vec);
		data_info_.time_point = pos_ratio;

		SendPreparedData();   // Успешное чтение - отправка данных
	}
	
	state_ = kNoProcess;  // Сброс состояния
}
