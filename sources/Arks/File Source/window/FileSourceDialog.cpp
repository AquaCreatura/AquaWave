#include "FileSourceDialog.h"
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <QTextStream>
#include "Tools/parse_tools.h"
#include <QtConcurrent/qtconcurrent_global.h>
#include <qfuture.h>
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
				SetFileName(current_file_name);
            }
        }
		
        connect(ui_.choose_path_tool_button, &QToolButton::clicked, this, &FileSourceDialog::OnChooseFilePath);
        connect(ui_.choose_path_line_edit, &QLineEdit::textChanged, [this](const QString& new_file_path)
        {   
            edit_file_info_.file_name_ = new_file_path;
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
        connect(ui_.ok_push_button, &QPushButton::clicked, this, &FileSourceDialog::OnOkButton);
    }
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

const bool file_source::FileSourceDialog::SetFileName(const QString & file_name)
{
	edit_file_info_ = SourceDescription();
    ParseFileName(file_name);
	if (file_name.endsWith(".wav", Qt::CaseInsensitive))
	{
		ParseWavHeader(file_name);
	}
    return true;
}

// Обработчик выбора файла
void file_source::FileSourceDialog::OnChooseFilePath()
{
	QString current_file_name = ui_.choose_path_line_edit->text();

	// Диалог выбора файла
	{
		// Изменён фильтр: теперь включает и .pcm, и .wav
		QString signal_filter = tr("Signals (*.pcm *.wav)");
		QString no_filter = tr("All files (*.*)");
		const QString set_of_filters = QString("%1;;%2").arg(signal_filter).arg(no_filter);

		// Проверка, является ли текущее имя файла сигнальным (pcm или wav)
		const bool is_signal_file = (current_file_name.size() > 5) &&
			(current_file_name.endsWith(".pcm", Qt::CaseInsensitive) ||
				current_file_name.endsWith(".wav", Qt::CaseInsensitive));
		QString& cur_filter = (is_signal_file || current_file_name.isEmpty()) ? signal_filter : no_filter;

		current_file_name = QFileDialog::getOpenFileName(this, tr("Choose file"),
			QFileInfo(current_file_name).absolutePath(), set_of_filters, &cur_filter);
		if (current_file_name.isEmpty()) return;
		
		SetFileName(current_file_name);
	}
}

void file_source::FileSourceDialog::ParseWavHeader(const QString & file_name)
{
	aqua_parse_tools::WavInfo info;
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
		}
	}
}

bool file_source::FileSourceDialog::ParseFileName(const QString& file_name)
{
    // Обновление пути в интерфейсе
    {
        ui_.choose_path_line_edit->setText(file_name);

        edit_file_info_.file_name_ = file_name;
    }
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

	if (QFileInfo::exists(edit_file_info_.file_name_) && (edit_file_info_.count_of_samples <= 0))
	{
		edit_file_info_.count_of_samples = QFile(edit_file_info_.file_name_).size() / GetSampleSize(edit_file_info_.data_type_);
	}
	return is_succesfully_parsed;
	

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
void file_source::FileSourceDialog::OnOkButton()
{
	if (!QFileInfo::exists(edit_file_info_.file_name_))
	{
		QMessageBox::critical(this, tr("No file error"),
			tr("Can not find specified file!\n\"%1\"").arg(edit_file_info_.file_name_));
		return;
	}
	aqua_parse_tools::WavInfo info;

	edit_file_info_.count_of_samples = (QFile(edit_file_info_.file_name_).size() - edit_file_info_.first_sample_offset) / GetSampleSize(edit_file_info_.data_type_);

	RememberFilePath();
    UpdateSourceNeed();
    this->close();
}