#include "FileSource.h"
#include <qglobal.h>
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <filesystem>
#include <QTextStream>
#include "Tools/parse_tools.h"
using namespace file_source;

// ========================================== FileSourceArk =================================

// Конструктор: создает диалог и инициализирует менеджер слушателей
file_source::FileSourceArk::FileSourceArk():
    listener_man_(file_info_)  // Инициализация менеджера с параметрами файла
{
    dialog_ = std::make_shared<FileSourceDialog>();  // Создание диалогового окна
    this->file_info_ = dialog_->GetFileInfo();
    connect(dialog_.get(), &FileSourceDialog::UpdateSourceNeed, [this]()
    {
        this->file_info_ = dialog_->GetFileInfo();
    });
    
}

file_source::FileSourceArk::~FileSourceArk()
{
}

// Обработчик сообщений (Dove - "голубь" как сообщение)
bool file_source::FileSourceArk::SendDove(fluctus::DoveSptr const& sent_dove)
{
    if (!sent_dove)
        throw std::invalid_argument("empty dove sent!");
    const auto &target_ark = sent_dove->target_ark ? sent_dove->target_ark: sent_dove->sender;
    const auto parrent_type = sent_dove->base_thought;
    
    // Обработка базовых команд
    if (parrent_type & fluctus::DoveParrent::kTieFront)
    {
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
            const auto carrier_hz    = file_src_dove->carrier_hz    ? *file_src_dove->carrier_hz    : file_info_.carrier_hz_;
            const auto samplerate_hz = file_src_dove->samplerate_hz ? *file_src_dove->samplerate_hz : file_info_.samplerate_hz_;
            listener_man_.InitReader(target_ark, carrier_hz, samplerate_hz, *file_src_dove->data_size);
        }
        
        // Запрос данных вокруг точки
        if (file_src_thought & FileSrcDove::FileSrcDoveThought::kAskSingleDataAround)
        {
            listener_man_.ReadAround(target_ark, *file_src_dove->time_point_start);
        }
        
        // Запрос циклических данных (не реализовано)
        if (file_src_thought & FileSrcDove::FileSrcDoveThought::kAskCyclicData)
        {
            //Do smth
        }
        
        // Запрос данных в диапазоне (не реализовано)
        if (file_src_thought & FileSrcDove::FileSrcDoveThought::kAskSingleDataInRange)
        {
            //Do smth
        }
    }
    return ArkBase::SendDove(sent_dove);
}

fluctus::ArkType file_source::FileSourceArk::GetArkType() const
{
    return fluctus::ArkType::kFileSource;
}

// Отправка данных (не реализована)
bool file_source::FileSourceArk::SendData(fluctus::DataInfo const& data_info)
{
    return false;
}

// ============================== FileSourceDialog ===================================

// Конструктор диалогового окна
file_source::FileSourceDialog::FileSourceDialog()
{
    ui_.setupUi(this);
    
    // Настройка обработчиков для типа данных
    {
        connect(ui_.is_data_complex_checkbox, &QCheckBox::clicked, this, &FileSourceDialog::UpdateDataTypes);
        connect(ui_.data_types_radio_group, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), 
                this, &FileSourceDialog::OnDataTypeChanged);
    }
    
    // Настройка обработчиков для пути к файлу
    {
        {
            QString current_file_name;
            QString last_path_file_name = QFileInfo(QCoreApplication::applicationFilePath())
                                          .dir().absoluteFilePath("file_read_path.txt");
            QFile file(last_path_file_name);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                current_file_name = in.readLine(); // читаем первую строку
                file.close();
                SetFileName(current_file_name);
            }
        }
        connect(ui_.choose_path_tool_button, &QToolButton::clicked, this, &FileSourceDialog::OnChooseFilePath);
        connect(ui_.choose_path_line_edit, &QLineEdit::textChanged, [this](const QString& new_file_path)
        {   
            file_info_.file_name_ = new_file_path;
        });  
    }
    
    // Настройка обработчиков параметров сигнала
    {
        connect(ui_.signal_settings_groupbox, &QGroupBox::clicked, [this]()
        {
            file_info_.is_signal_type = ui_.signal_settings_groupbox->isChecked();
        });
        connect(ui_.carrier_mhz_spinbox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), 
                [this](const double new_val)
        {
            file_info_.carrier_hz_ = std::llround(new_val * 1'000'000);  // MHz -> Hz
        });
        connect(ui_.samplerate_khz_spinbox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), 
                [this](const double new_val)
        {
            file_info_.samplerate_hz_ = std::llround(new_val * 1'000);  // kHz -> Hz
        });
    }
    
    // Обработчик кнопки OK
    {
        connect(ui_.ok_push_button, &QPushButton::clicked, this, &FileSourceDialog::OnOkButton);
    }
}

file_source::FileSourceDialog::~FileSourceDialog()
{
    //File read path
    {
        QString last_path_file_name = QFileInfo(QCoreApplication::applicationFilePath())
                                    .dir().absoluteFilePath("file_read_path.txt");
        QFile file(last_path_file_name);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) 
        {
            QTextStream out(&file);
            out << file_info_.file_name_;
            file.close();
        }
    }
}

// Получение текущих параметров файла
const file_params& file_source::FileSourceDialog::GetFileInfo() const
{
    return file_info_;
}

// Обработчик выбора файла
void file_source::FileSourceDialog::OnChooseFilePath()
{
    QString current_file_name = ui_.choose_path_line_edit->text();  
    
    // Диалог выбора файла
    {
        QString pcm_filter = tr("Signals (*.pcm)");
        QString no_filter = tr("All files (*.*)");
        const QString set_of_filters = QString("%1;;%2").arg(pcm_filter).arg(no_filter);
        const bool is_pcm_file = ((current_file_name.size() > 5) && current_file_name.endsWith(".pcm"));
        QString& cur_filter = (is_pcm_file || current_file_name.isEmpty()) ? pcm_filter : no_filter;

        current_file_name = QFileDialog::getOpenFileName(this, tr("Choose file"),
                            QFileInfo(current_file_name).absolutePath(), set_of_filters, &cur_filter);
        if(current_file_name.isEmpty()) return;
    }
    SetFileName(current_file_name);
    UpdateSourceNeed();
}

void file_source::FileSourceDialog::SetFileName(const QString& file_name)
{
    // Обновление пути в интерфейсе
    {
        ui_.choose_path_line_edit->setText(file_name);
        file_info_.file_name_ = file_name;
    }
    
    // Парсинг параметров из имени файла (формат: "... 869.977996MHz 219.999KHz.pcm")
    bool is_succesfully_parsed = true;
    int64_t samplerate_hz = 0, carrier_hz = 1.e6;
    if(aqua_parse_tools::get_samplerate_from_filename(file_name.toStdString(),file_info_.samplerate_hz_))
    {
        is_succesfully_parsed = true;
        aqua_parse_tools::get_carrier_from_filename(file_name.toStdString(), file_info_.carrier_hz_);
    }
    ui_.signal_settings_groupbox->setChecked(is_succesfully_parsed); //Факт того, что успешно достали ЧД, нас более чем удовлетворяет
    ui_.carrier_mhz_spinbox     ->setValue(file_info_.carrier_hz_   / 1.e6);
    ui_.samplerate_khz_spinbox  ->setValue(file_info_.samplerate_hz_/ 1.e3);

}

// Обновление текста типов данных (real/complex)
void file_source::FileSourceDialog::UpdateDataTypes()
{
    bool is_complex_type = ui_.is_data_complex_checkbox->isChecked();
    QString suffix = is_complex_type ? "complex" : "real"; 
    ui_.float32_radio_button->setText(QString("float 32 ") + suffix);
    ui_.int16_radio_button->setText(QString("signed 16 ") + suffix);
    ui_.int8_radio_button->setText(QString("signed 8 ") + suffix);

    OnDataTypeChanged();
}

// Обработчик изменения типа данных
void file_source::FileSourceDialog::OnDataTypeChanged()
{
    bool is_complex = ui_.is_data_complex_checkbox->isChecked();

    if (ui_.float32_radio_button->isChecked()) {
        file_info_.data_type_ = is_complex ? ipp32fc : ipp32f;
    } else if (ui_.int16_radio_button->isChecked()) {
        file_info_.data_type_ = is_complex ? ipp16sc : ipp16s;
    } else if (ui_.int8_radio_button->isChecked()) {
        file_info_.data_type_ = is_complex ? ipp8sc : ipp8s;
    }
} 

// Обработчик кнопки OK
void file_source::FileSourceDialog::OnOkButton()
{
    if (!QFileInfo::exists(file_info_.file_name_))
    {
        QMessageBox::critical(this, tr("No file error"), 
                          tr("Can not find specified file!\n\"%1\"").arg(file_info_.file_name_));
        return;
    }
    this->close();
}