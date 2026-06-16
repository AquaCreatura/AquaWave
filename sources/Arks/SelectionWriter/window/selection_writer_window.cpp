#include "selection_writer_window.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

const char* txt_name = "out_folder_path.txt";
SelectionWriterWindow::SelectionWriterWindow()
{
	ui_.setupUi(this);
	connect(ui_.start_record_button, &QPushButton::clicked, this, &SelectionWriterWindow::OnStartButtonClicked);
	connect(ui_.file_path_choose_button, &QPushButton::clicked, this, &SelectionWriterWindow::OnChoosePathButton);

	{
		QFile file(QCoreApplication::applicationDirPath() + "/" + txt_name);
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
			ui_.file_path_line_edit->setText(QTextStream(&file).readAll().trimmed());
	}
}

void SelectionWriterWindow::SetFreqparams(int64_t carrier_hz, int64_t samplerate_hz)
{
	ui_.carrier_spinbox_MHz->setValue(carrier_hz / 1.e6);
	ui_.samplerate_spinbox_kHz->setValue(samplerate_hz / 1.e3);
}

void SelectionWriterWindow::Stop()
{
	if (!IsStarted()) return;
	OnStartButtonClicked();
}

void SelectionWriterWindow::UpdateProgressRatio(const double status_ratio)
{
	ui_.progress_bar->setValue(status_ratio * ui_.progress_bar->maximum());
}

void SelectionWriterWindow::UpdateBytesWritten(size_t bytes_written)
{
	ui_.mbytes_written_spinbox->setValue(bytes_written / 1.e6);
}


void SelectionWriterWindow::OnChoosePathButton()
{
	const QString currentPath = ui_.file_path_line_edit->text();

	const QString selectedDir = QFileDialog::getExistingDirectory(
		this,
		tr("Choose folder"),
		currentPath,
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (!selectedDir.isEmpty())
	{
		ui_.file_path_line_edit->setText(selectedDir);
	}


	{
		QFile file(QCoreApplication::applicationDirPath() + "/" + txt_name);
		if (file.open(QIODevice::WriteOnly | QIODevice::Text))
			QTextStream(&file) << selectedDir;
	}
}

void SelectionWriterWindow::OnStartButtonClicked()
{
	const bool need_start = (IsStarted() == false);
	if (need_start) {
		const auto line_edit_text = ui_.file_path_line_edit->text().toLocal8Bit();
		std::string file_path(line_edit_text.data(), line_edit_text.data() + line_edit_text.size());
		StartNeed(file_path);
	}
	else {
		StopNeed();
	}
	const bool is_started = IsStarted();
	ui_.start_record_button->setChecked(is_started);
	ui_.start_record_button->setText(is_started ? "Stop" : "Start");

}
//=========================================== FileSavedDialog ======================================
FileSavedDialog::FileSavedDialog(const std::string& file_path,
	const size_t file_size_bytes)
{
	ui_.setupUi(this);
	ui_.file_size_mbyte_spinbox->setValue( static_cast<double>(file_size_bytes) / 1.e6);

	QString path(file_path.c_str());

	ui_.file_path_label->setText(path);

	// Разрешаем выделение и копирование
	ui_.file_path_label->setTextInteractionFlags(
		Qt::TextSelectableByMouse |
		Qt::TextSelectableByKeyboard);

	// Показываем полный путь во всплывающей подсказке
	ui_.file_path_label->setToolTip(path);

	// Подгоняем ширину под текст, но не более 400 px
	int width = ui_.file_path_label->sizeHint().width();
	ui_.file_path_label->setFixedWidth(std::min(width, 400));
	ui_.file_path_label->setWordWrap(true);
}
