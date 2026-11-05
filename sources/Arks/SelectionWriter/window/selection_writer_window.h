#pragma once
#include <qdialog.h>
#include "ui_file_saved_dialog.h"
#include "ui_selection_writer_window.h"

class SelectionWriterWindow: public QDialog
{
	Q_OBJECT
public:
	SelectionWriterWindow();
	void SetFreqparams(int64_t carrier_hz, int64_t samplerate_hz, int64_t bandwidth_hz, double bw_ratio);
	int64_t GetSamplerateHz() const;
	int64_t GetBandwidthHz () const;
	int64_t GetCarrierHz   () const;
	void Stop();
	void UpdateProgressRatio(const double status_ratio);
	void UpdateBytesWritten	(size_t bytes_written);
signals:
	bool StartNeed(std::string path_record);
	bool StopNeed();
	bool IsStarted();
protected:
	void OnChoosePathButton();
	void OnStartButtonClicked();
protected:
	void UpdateBandwidthLimit(double samplerate_khz);
protected:
	Ui::SelectionWriterWindow ui_;
	double bw_ratio_{ 0.9 };
};


class FileSavedDialog : public QDialog
{
	Q_OBJECT
public:
	FileSavedDialog(const std::string& file_path, const size_t file_size_bytes);
protected:
	Ui::file_saved_dialog ui_;
};



