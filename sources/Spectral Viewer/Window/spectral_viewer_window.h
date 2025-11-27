#pragma once
#include "ui_spectral_viewer_window.h"
#include <qdialog.h>



//using namespace fluctus;


class SpectralViewerWindow: public QDialog
{
Q_OBJECT
public:
	SpectralViewerWindow();
    void SetChartWindow(QWidget* wigdet_ptr);
	void SetMaxFFtOrder(int n_fft_order);
signals:
	void FftChangeNeed(int new_fft);
protected:
	void UpdateFFtCombobox(const int max_order, const int cur_fft_order);
protected:
    Ui::spectral_viewer_window ui_;
};

