#include "FileDataManager.h"
#include "FileReadCore.h"
using namespace file_source;

//=================================== FileDataManager Implementation =============================

// Конструктор: инициализирует параметры файла
file_source::FileDataManager::FileDataManager(const file_source::file_params& params) : 
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
    listener.StopProcess();              // Останавливает текущий процесс (если работает)
    listener.SetBaseParams(carrier, samplerate, block_size);  // Настраивает базовые параметры
}

// Запуск чтения вокруг позиции для указанного ARK
void file_source::FileDataManager::ReadAround(const fluctus::ArkWptr& reader, const double pos_ratio)
{
    auto& listener = listeners_.try_emplace(reader, reader, params_).first->second;  // Получает listener (должен быть инициализирован)
    listener.StartReadAround(pos_ratio);  // Асинхронный запуск чтения
}

// Удаление слушателя для указанного ARK
void file_source::FileDataManager::DeleteReader(const fluctus::ArkWptr& reader)
{
    auto it = listeners_.find(reader);
    if(it == listeners_.end()) return;   // Выход, если listener не найден
    it->second.StopProcess();           // Остановка процесса перед удалением
    listeners_.erase(it);                // Удаление из map
}

//=================================== FileDataListener Implementation ============================

// Конструктор: инициализирует weak_ptr на ARK и параметры файла
file_source::FileDataListener::FileDataListener(const fluctus::ArkWptr& weak_ptr, const file_params& params):
    target_ark_(weak_ptr),  // Инициализация ссылки на weak_ptr
    params_(params)         // Инициализация константной ссылки на параметры
{
}

// Установка базовых параметров (частота, sample rate)
void file_source::FileDataListener::SetBaseParams(const int64_t carrier, 
                                                const int64_t samplerate, 
                                                const int64_t block_size)
{
    data_info_.freq_info_.carrier = carrier;      // Установка несущей частоты
    data_info_.freq_info_.samplerate = samplerate; // Установка частоты дискретизации
    block_size_ = block_size;                     // Сохранение размера блока
}

// Остановка текущего асинхронного процесса
void file_source::FileDataListener::StopProcess()
{
    if(process_anchor_.valid()) 
        process_anchor_.get();  // Блокирует, пока поток не завершится
}

// Асинхронный запуск чтения вокруг позиции
bool file_source::FileDataListener::StartReadAround(const double pos_ratio)
{  
    // Запуск ReadAroundProcess в отдельном потоке
    process_anchor_ = std::async(std::launch::async, 
                                &FileDataListener::ReadAroundProcess, 
                                this, 
                                pos_ratio, 
                                block_size_);
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

// Основной процесс чтения данных вокруг позиции
void file_source::FileDataListener::ReadAroundProcess(double pos_ratio, const int64_t out_data_size)
{
    FileReader reader;
    if(!reader.SetFileParams(params_))  // Настройка файлового читателя
    {
        // Можно добавить callback для ошибки (не критическая ошибка)
        return;
    }
    
    state_ = kReadAround;  // Установка состояния "в процессе чтения"
    
    // Чтение данных вокруг позиции
    if(!reader.GetDataAround(pos_ratio, out_data_size, data_info_.data_vec)) 
    {
        state_ = kProcessStopped;  // Ошибка чтения
        return;
    }
    
    std::vector<Ipp32fc> data_vec(out_data_size);

    ippsConvert_16s32f((Ipp16s*)data_info_.data_vec.data(), (Ipp32f*)data_vec.data(), out_data_size * 2);
    data_info_.data_vec.swap((std::vector<uint8_t>&)data_vec);


    SendPreparedData();   // Успешное чтение - отправка данных
    state_ = kNoProcess;  // Сброс состояния
}