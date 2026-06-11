#pragma once
#include "ui_selection_writer_window.h"
#include <qdialog.h>


class SelectionWriterWindow: public QDialog
{
	Q_OBJECT
public:
	SelectionWriterWindow();
	void SetFreqparams(int64_t carrier_hz, int64_t samplerate_hz);
	void Stop();
	void UpdateProgressRatio(const double status_ratio);
	void UpdateBytesWritten	(size_t bytes_written);
signals:
	bool StartNeed(std::string path_record);
	bool StopNeed();
	bool IsStarted();
protected:
	void OnStartButtonClicked();
protected:
	Ui::SelectionWriterWindow ui_;
};

