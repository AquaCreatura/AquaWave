#include "FileSourceDialog.h"

#include "Utilities/file_helpers.h"
#include "Utilities/parse_tools.h"
#include "Utilities/qt_utility.h"
#include "Utilities/wav_tools.h"

#include <qfiledialog.h>
#include <qmessagebox.h>
#include <QTextStream>
#include "GUI/External/Window/QWindowKitHelper.h"
using namespace file_source;

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
				InitFromFileName(current_file_name);
            }
        }
		
		connect(ui_.choose_path_tool_button, &QPushButton::clicked, this, &FileSourceDialog::OnChooseFilePath);
        connect(ui_.choose_path_line_edit, &QLineEdit::textChanged, [this](const QString& new_file_path)
        {   
            path_edit_debouncer_.Call([this, new_file_path]()
            {
                edit_file_info_.file_name_ = new_file_path;
                InitFromFileName(new_file_path);
            });
        });  
    }
    
    // Настройка обработчиков параметров сигнала
    {
        connect(ui_.signal_settings_groupbox, &QGroupBox::clicked, [this]()
        {
            edit_file_info_.is_signal = ui_.signal_settings_groupbox->isChecked();
        });
        connect(ui_.carrier_mhz_spinbox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), 
                [this](const double new_val)
        {
            edit_file_info_.carrier_hz = std::llround(new_val * 1'000'000);  // MHz -> Hz
        });
        connect(ui_.samplerate_khz_spinbox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), 
                [this](const double new_val)
        {
            edit_file_info_.samplerate_hz = std::llround(new_val * 1'000);  // kHz -> Hz
        });
    }
    
        // Обработчик кнопки OK
        {
            connect(ui_.ok_push_button, &QPushButton::clicked, this, &FileSourceDialog::StartProcessingParams);
        }

    const QString img_base = ":/buttons/button_images/";
    ui_.ok_push_button->setText("");
    ui_.ok_push_button->setStyleSheet(buildButtonStyleSheet(
        50,
        img_base + "ok_water_button.png",
        img_base + "ok_water_button.png",
        img_base + "ok_water_button.png",
        img_base + "ok_water_button.png",
        img_base + "ok_water_button.png"));
}

file_source::FileSourceDialog::~FileSourceDialog()
{
	RememberFilePath();
}

// Получение текущих параметров файла
const SourceDescription& file_source::FileSourceDialog::GetFileInfo() const
{
    return edit_file_info_;
}

const bool file_source::FileSourceDialog::InitFromFileName(const QString & file_name)
{

	edit_file_info_ = SourceDescription();

    // Обновление пути в GUI
    {
        if (ui_.choose_path_line_edit->text() != file_name)
            ui_.choose_path_line_edit->setText(file_name);
        edit_file_info_.file_name_ = file_name;
    }


    bool is_good_parse = ParseFileName(file_name);
	if (file_name.endsWith(".wav", Qt::CaseInsensitive))
	{
        is_good_parse |= ParseWavHeader(file_name);
	}
    bool is_file_exist = UpdateDataInfo(file_name);
    return is_good_parse && is_file_exist;
}

// Обработчик выбора файла
void file_source::FileSourceDialog::OnChooseFilePath()
{
    QString current_file_name = ui_.choose_path_line_edit->text();

    // 1. Настройка фильтров
    const QString signal_filter = tr("Signals (*.pcm *.wav)");
    const QString no_filter = tr("All files (*.*)");
    const QString set_of_filters = QString("%1;;%2").arg(signal_filter).arg(no_filter);

    // 2. Определение текущего фильтра
    const bool is_signal_file = current_file_name.size() > 5 &&
        (current_file_name.endsWith(".pcm", Qt::CaseInsensitive) ||
            current_file_name.endsWith(".wav", Qt::CaseInsensitive));

    QString cur_filter = (is_signal_file || current_file_name.isEmpty()) ? signal_filter : no_filter;

    // 3. ГЛАВНОЕ ИЗМЕНЕНИЕ: 
    // Передаём полный путь к файлу (absoluteFilePath), чтобы Qt выделил его в диалоге.
    // Если имя пустое, передаём пустую строку (Qt откроет последнюю использованную или домашнюю папку).
    const QString start_path = current_file_name.isEmpty()
        ? QString()
        : QFileInfo(current_file_name).absoluteFilePath();

    // 4. Вызов диалога
    current_file_name = QFileDialog::getOpenFileName(
        this,
        tr("Choose file"),
        start_path,
        set_of_filters,
        &cur_filter
    );

    if (current_file_name.isEmpty()) {
        return; // Пользователь нажал "Отмена"
    }

    InitFromFileName(current_file_name);
}

bool file_source::FileSourceDialog::ParseWavHeader(const QString & file_name)
{
	utility_aqua::WavInfo info;
	if (get_wav_info(file_name.toLocal8Bit().constData(), info))
	{
		if (info.sample_rate && info.data_offset)
		{
			edit_file_info_.samplerate_hz = info.sample_rate.value();
			edit_file_info_.first_sample_offset = info.data_offset.value();
			edit_file_info_.count_of_samples = (QFile(edit_file_info_.file_name_).size() - edit_file_info_.first_sample_offset) / GetSampleSize(edit_file_info_.data_type_);
			// Несущая частота не меняется (берётся из имени файла)

			ui_.samplerate_khz_spinbox->setValue(edit_file_info_.samplerate_hz / 1e3);
			ui_.carrier_mhz_spinbox->setValue(edit_file_info_.carrier_hz / 1e6);
			ui_.signal_settings_groupbox->setChecked(true);
            return true;
		}
	}
    return false;
}

bool file_source::FileSourceDialog::ParseFileName(const QString& file_name)
{
    // Парсинг параметров из имени файла (формат: "... 869.977996MHz 219.999KHz.pcm")
    bool is_succesfully_parsed = false;
    int64_t samplerate_hz = 0, carrier_hz = 1.e6;
    if(aqua_parse_tools::get_samplerate_from_filename(file_name.toLocal8Bit().constData(),edit_file_info_.samplerate_hz))
    {
        is_succesfully_parsed = true;
        aqua_parse_tools::get_carrier_from_filename(file_name.toLocal8Bit().constData(), edit_file_info_.carrier_hz);
    }
    ui_.signal_settings_groupbox->setChecked(is_succesfully_parsed); //Факт того, что успешно достали ЧД, нас более чем удовлетворяет
    ui_.carrier_mhz_spinbox     ->setValue(edit_file_info_.carrier_hz   / 1.e6);
    ui_.samplerate_khz_spinbox  ->setValue(edit_file_info_.samplerate_hz/ 1.e3);
	return is_succesfully_parsed;
}

bool file_source::FileSourceDialog::UpdateDataInfo(const QString& file_name)
{
    bool is_file_exist = CheckFileConnection(file_name, 100);
    if (is_file_exist) {
        if (FsHelper::IsFile32Float(file_name.toLocal8Bit().constData()))
            ui_.float32_radio_button->setChecked(true);
        else
            ui_.int16_radio_button->setChecked(true);
        OnDataTypeChanged();

        edit_file_info_.count_of_samples = (QFile(edit_file_info_.file_name_).size() - edit_file_info_.first_sample_offset) / GetSampleSize(edit_file_info_.data_type_);
    }    
    ui_.ok_push_button->setVisible(is_file_exist);
    return is_file_exist;
}

void file_source::FileSourceDialog::RememberFilePath()
{
	//File read path
	QString last_path_file_name = QFileInfo(QCoreApplication::applicationFilePath())
		.dir().absoluteFilePath("file_read_path.txt");
	QFile file(last_path_file_name);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream out(&file);
		out << edit_file_info_.file_name_;
		file.close();
	}
	
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
        edit_file_info_.data_type_ = is_complex ? ipp32fc : ipp32f;
    } else if (ui_.int16_radio_button->isChecked()) {
        edit_file_info_.data_type_ = is_complex ? ipp16sc : ipp16s;
    } else if (ui_.int8_radio_button->isChecked()) {
        edit_file_info_.data_type_ = is_complex ? ipp8sc : ipp8s;
    }
} 

// Обработчик кнопки OK
void file_source::FileSourceDialog::StartProcessingParams()
{
    InitFromFileName(ui_.choose_path_line_edit->text());
	if (edit_file_info_.count_of_samples <= 0)
	{
		QMessageBox::critical(this, tr("No file error"), tr("Can not find specified file!\n\"%1\"").arg(edit_file_info_.file_name_));
		return;
	}	
	RememberFilePath();
    UpdateSourceNeed();
    this->close();
}
