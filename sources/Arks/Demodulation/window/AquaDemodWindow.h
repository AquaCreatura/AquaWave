#pragma once
#include "ui_AquaDemodWindow.h"
#include <qdialog.h>



//using namespace fluctus;


class AquaDemodWindow : public QDialog
{
	Q_OBJECT
public:
	AquaDemodWindow();
protected:
	Ui::AquaDemodWindow ui_;
};