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
	signals:
	void FftChangeNeed(int new_fft);
protected:
    Ui::SpgWindow ui_;
};

