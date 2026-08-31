#include "spectral_viewer_window.h"
#include "Utilities/parse_tools.h"
#include <qshortcut.h>
#include "GUI/Charts/ChartInterface.h"
SpectralViewerWindow::SpectralViewerWindow()
{
    ui_.setupUi(this);
	//Îןנוהוכÿול Combobox הכÿ FFT
	{
		connect(ui_.fft_order_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
			int fft_id = ui_.fft_order_combobox->itemData(index).toInt();
			emit FftChangeNeed(fft_id);
		});
		QShortcut* saveShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_S), this);
		connect(saveShortcut, &QShortcut::activated, this, &SpectralViewerWindow::RecordSelectionNeed);

		UpdateFFtCombobox(21, 10);
	}
	
};

void SpectralViewerWindow::SetDpxSpectrumWindow(QWidget * wigdet_ptr)
{
	ui_.dpx_chart->layout()->addWidget(wigdet_ptr);


	
}
void SpectralViewerWindow::SetSpectrogramWindow(QWidget * wigdet_ptr)
{
	ui_.spg_chart->layout()->addWidget(wigdet_ptr);
	qobject_cast<ChartInterface*>(wigdet_ptr)->SetControlButtons(ui_.ctrl_buttons_frame);

}
void SpectralViewerWindow::SetMaxFFtOrder(int max_fft_order)
{
	const auto cur_fft = ui_.fft_order_combobox->currentData().toInt();
	UpdateFFtCombobox(max_fft_order, cur_fft);
}
void SpectralViewerWindow::UpdateFFtCombobox(const int max_order, const int cur_fft_order)
{
	{
		QSignalBlocker blocker(ui_.fft_order_combobox);
		ui_.fft_order_combobox->clear();
		for (int fft_counter = 4; fft_counter <= max_order; fft_counter++) {
			QString item_text = QString("%1").arg(aqua_parse_tools::ValueToString(1 << fft_counter, 0, " ").c_str());
			ui_.fft_order_combobox->addItem(item_text, fft_counter);
		}
	}
	{
		int target_fft = cur_fft_order;
		int index = ui_.fft_order_combobox->findData(target_fft);
		if (index != -1) {
			{
				QSignalBlocker blocker(ui_.fft_order_combobox);
				ui_.fft_order_combobox->setCurrentIndex(index);  // גûחמגוע emit currentIndexChanged
			}
			emit ui_.fft_order_combobox->currentIndexChanged(index);
		}

	}
}






