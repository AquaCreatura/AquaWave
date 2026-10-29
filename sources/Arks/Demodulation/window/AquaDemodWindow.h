#pragma once
#include "Utilities/utility_aqua.h"

#include <qdialog.h>
#include "ui_AquaDemodWindow.h"

//using namespace fluctus;
using namespace utility_aqua;

class AquaDemodWindow : public QDialog
{
	Q_OBJECT
public:
	AquaDemodWindow();
	void SetFreqParams(aqua_opt<double> carrier_hz, aqua_opt<double> symbol_rate_hz);
protected:
	Ui::AquaDemodWindow ui_;
};
