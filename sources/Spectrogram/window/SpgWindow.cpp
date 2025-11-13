#include "SpgWindow.h"
#include "Tools\parse_tools.h"
SpgWindow::SpgWindow()
{
    ui_.setupUi(this);
	for (int fft_counter = 8; fft_counter <= 20; fft_counter++) {
		QString item_text = QString("%1 (%2)").arg(aqua_parse_tools::ValueToString(1 << fft_counter, 0, " ").c_str()).arg(fft_counter);
		ui_.fft_combobox->addItem(item_text, fft_counter);
	}
};
void SpgWindow::SetChartWindow(QWidget * wigdet_ptr)
{
    ui_.SpgChart->layout()->addWidget(wigdet_ptr);
	
}

