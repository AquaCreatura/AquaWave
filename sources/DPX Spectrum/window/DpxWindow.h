#pragma once
#include "ui_DpxWindow.h"
#include <qdialog.h>



//using namespace fluctus;


class DpxWindow : public QDialog
{
Q_OBJECT
public:
    DpxWindow();
    void SetChartWindow(QWidget* wigdet_ptr);
	void SetMaxFFtOrder(int n_fft_order);
signals:
	void FftChangeNeed(int new_fft);
protected:
	void UpdateFFtCombobox(const int max_order, const int cur_fft_order);
protected:
    Ui::DpxWindow ui_;
};

