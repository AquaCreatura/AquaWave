#pragma once
#include "ui_SpgWindow.h"
#include <qdialog.h>



//using namespace fluctus;


class SpgWindow : public QDialog
{
Q_OBJECT
public:
    SpgWindow();
    void SetChartWindow(QWidget* wigdet_ptr);
	void SetMaxFFtOrder(int n_fft_order);
	signals:
	void FftChangeNeed(int new_fft);
protected:
	void UpdateFFtCombobox(const int max_order, const int cur_fft_order);
    Ui::SpgWindow ui_;
};

