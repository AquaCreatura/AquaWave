#include "FileSource.h"

using namespace file_source;
using namespace fluctus;
// ========================================== FileSourceArk =================================


file_source::FileSourceArk::FileSourceArk(QWidget * main_window) :
	listener_man_(file_info_),  // Инициализация менеджера с параметрами файла
	qmain_window_(main_window)
{
	dialog_ = new FileSourceDialog;  // Создание диалогового окна
	connect(dialog_, &FileSourceDialog::UpdateSourceNeed, this, &FileSourceArk::UpdateSource);
	UpdateSource();
}

file_source::FileSourceArk::~FileSourceArk()
{
	listener_man_.StopAllReaders();
}

// Обработчик сообщений (Dove - "голубь" как сообщение)
bool file_source::FileSourceArk::SendDove(fluctus::DoveSptr const& sent_dove)
{
    if (!sent_dove)
        throw std::invalid_argument("empty dove sent!");
    const auto &target_ark = sent_dove->target_ark ? sent_dove->target_ark: sent_dove->sender;
    const auto parrent_type = sent_dove->base_thought;
    
    // Обработка базовых команд
    if (parrent_type & fluctus::DoveParrent::kTieSink)
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
    
    // Запрос диалогового окна
    if (parrent_type & fluctus::DoveParrent::kGetDialog)
    {
        sent_dove->show_widget = dialog_;  // Возвращаем указатель на диалог
    }
    

    // Обработка специализированных команд для файлового источника
    if (parrent_type & fluctus::DoveParrent::kSpecialThought)
    {
        auto file_src_dove = std::dynamic_pointer_cast<FileSrcDove>(sent_dove);
        if (!file_src_dove)
            throw std::invalid_argument("wrong thought type!");
            
        const auto file_src_thought = file_src_dove->special_thought;
        
        // Инициализация читателя
        if (file_src_thought & FileSrcDove::FileSrcDoveThought::kInitReaderInfo)
        {
			const auto carrier_hz		= file_src_dove->carrier_hz		.value_or(file_info_.carrier_hz	);
			const auto samplerate_hz	= file_src_dove->samplerate_hz	.value_or(file_info_.samplerate_hz	);
            listener_man_.InitReader(target_ark, carrier_hz, samplerate_hz, *file_src_dove->data_size);
        }
        
        // Запрос данных вокруг точки
        if (file_src_thought & FileSrcDove::FileSrcDoveThought::kAskChunkAround)
        {
			if (file_info_.file_name_.isEmpty()) return false;
            listener_man_.StartReading(target_ark, *file_src_dove->time_point_start, *file_src_dove->time_point_start, FileDataManager::kReadAround);
        }
        
        // Запрос данных чанками в диапазоне (единожды)
        if (file_src_thought & FileSrcDove::FileSrcDoveThought::kAskChunksInRange)
        {
			listener_man_.StartReading(target_ark, *file_src_dove->time_point_start, *file_src_dove->time_point_end, FileDataManager::kReadChunksInRange);
        }
        
        // Запрос данных в диапазоне (не реализовано)
        if (file_src_thought & FileSrcDove::FileSrcDoveThought::kAskWholeInRange)
        {
            //Do smth
        }
        if (file_src_thought & FileSrcDove::FileSrcDoveThought::kSetFileName)
        {
            const QString file_name = (*file_src_dove->file_info).file_name_;
			UpdateSource();
            dialog_->SetFileName(file_name);
            //Do smth 
        }//kSetFile
        if (file_src_thought & FileSrcDove::FileSrcDoveThought::kGetFileInfo)
        {
			if (file_info_.file_name_.isEmpty()) return false;
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
	listener_man_.StopAllReaders();
    this->file_info_ = dialog_->GetFileInfo(); //Update info, according ui
	if (qmain_window_) qmain_window_->setWindowTitle(tr("[AquaWave v.1.0] %1").arg(file_info_.file_name_));
    //Reset out arks
    {
        auto out_fleet   = GetFrontArks();
        fluctus::DoveSptr message   = std::make_shared<fluctus::DoveParrent>();
        message->base_thought       = DoveParrent::kReset;
        for(auto &out_ark: out_fleet) out_ark->SendDove(message);
    }
}

// Отправка данных (не реализована)
bool file_source::FileSourceArk::SendData(fluctus::DataInfo const& data_info)
{
    return false;
}
