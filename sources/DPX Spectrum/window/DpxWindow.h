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
signals:
	void FftChangeNeed(int new_fft);
protected:
    Ui::DpxWindow ui_;
};

