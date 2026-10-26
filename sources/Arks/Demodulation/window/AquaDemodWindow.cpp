#include "AquaDemodWindow.h"

AquaDemodWindow::AquaDemodWindow()
{
	ui_.setupUi(this);
}

void AquaDemodWindow::SetFreqParams(aqua_opt<double> carrier_hz, aqua_opt<double> symbol_rate_hz)
{
	if (carrier_hz) {
		ui_.fc_mhz_spinbox->setValue(*carrier_hz / 1.e6);
	}
	if (symbol_rate_hz) {
		ui_.bd_khz_spinbox->setValue(*symbol_rate_hz / 1.e3);
	}
}
