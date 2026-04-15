#include "FileSource.h"

using namespace file_source;
using namespace fluctus;
// ========================================== FileSourceArk =================================
constexpr double bw_filter_koeff_c_expr = 0.9;

file_source::FileSourceArk::FileSourceArk(QWidget * main_window) :
	listener_man_(descr_),  // Инициализация менеджера с параметрами файла
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
    
	if (parrent_type & FileSrcDove::kGetDescription)
	{
		if (descr_.file_name_.isEmpty()) return false;
		sent_dove->description = this->descr_;
		//Do smth
	}
    // Обработка специализированных команд для файлового источника
    if (parrent_type & fluctus::DoveParrent::kSpecialThought)
    {
        auto file_src_dove = std::dynamic_pointer_cast<FileSrcDove>(sent_dove);
        if (!file_src_dove)
            throw std::invalid_argument("wrong thought type!");
            
        const auto file_src_thought = file_src_dove->special_thought;
        
        // Инициализация читателя
		if (file_src_thought & FileSrcDove::FileSrcDoveThought::kInitiate)
		{
			listener_man_.InitReader(target_ark, *file_src_dove->setup);
		}
        if (file_src_thought & FileSrcDove::FileSrcDoveThought::kSetChunkSize)
        {
            listener_man_.UpdateChunkSize(target_ark, file_src_dove->setup->chunk_size);
        }
        
        // 
		for (auto read_type : { FileSrcDove::kAskChunkAround, FileSrcDove::kAskChunksInRange, FileSrcDove::kAskLoopInRange }) {
			if (file_src_thought & read_type){
				listener_man_.StartReading(target_ark, file_src_dove->time_bounds, read_type);
			}
		}

        if (file_src_thought & FileSrcDove::FileSrcDoveThought::kSetFileName)
        {
            const QString file_name = file_src_dove->description->file_name_;
			UpdateSource();
            dialog_->SetFileName(file_name);
			dialog_->setStyleSheet(qmain_window_->styleSheet());
			
			dialog_->exec();
        }//kSetFile
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
    this->descr_ = dialog_->GetFileInfo(); //Update descr, according ui
	descr_.bw_ratio_ = bw_filter_koeff_c_expr;
	if (qmain_window_) qmain_window_->setWindowTitle(tr("[AquaWave v.2.0]   %1").arg(descr_.file_name_));
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
