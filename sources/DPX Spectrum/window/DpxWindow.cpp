#include "DpxWindow.h"
#include "Tools\parse_tools.h"
DpxWindow::DpxWindow()
{
    ui_.setupUi(this);
	//Îןנוהוכÿול Combobox הכÿ FFT
	{
		for (int fft_counter = 6; fft_counter <= 20; fft_counter++) {
			QString item_text = QString("%1 (%2)").arg(aqua_parse_tools::ValueToString(1 << fft_counter, 0, " ").c_str()).arg(fft_counter);
			ui_.fft_combobox->addItem(item_text, fft_counter);
		};
		connect(ui_.fft_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
			int fft_id = ui_.fft_combobox->itemData(index).toInt();
			emit FftChangeNeed(fft_id);
		});
		{
			int target_fft = 10;
			int index = ui_.fft_combobox->findData(target_fft);
			if (index != -1) {
				ui_.fft_combobox->setCurrentIndex(index);  // גûחמגוע emit currentIndexChanged
			}
		}
	}
	
};

void DpxWindow::SetChartWindow(QWidget * wigdet_ptr)
{
    ui_.DpxChart->layout()->addWidget(wigdet_ptr);
    
}
