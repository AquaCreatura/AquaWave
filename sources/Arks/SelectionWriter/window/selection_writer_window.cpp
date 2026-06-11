#include "selection_writer_window.h"

SelectionWriterWindow::SelectionWriterWindow()
{
	ui_.setupUi(this);
	connect(ui_.start_record_button, &QPushButton::clicked, this, &SelectionWriterWindow::OnStartButtonClicked);

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

void SelectionWriterWindow::OnStartButtonClicked()
{
	const bool need_start = (IsStarted() == false);
	if (need_start) {
		const auto line_edit_text = ui_.file_path_line_edit->text().toUtf8();
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
