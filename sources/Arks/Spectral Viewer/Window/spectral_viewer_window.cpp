#include "spectral_viewer_window.h"
#include "Utilities/parse_tools.h"
#include <qshortcut.h>
#include "GUI/Charts/ChartInterface.h"
#include "Arks/Interfaces/ark_interface.h"
using namespace spectral_viewer;
SpectralViewerWindow::SpectralViewerWindow()
{
    ui_.setupUi(this);
	//Определяем Combobox для FFT
	{
		connect(ui_.fft_order_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
			int fft_id = ui_.fft_order_combobox->itemData(index).toInt();
			emit FftChangeNeed(fft_id);
		});
		connect(ui_.analysis_side_pushbutton, &QPushButton::clicked, [this]() {
			ui_.main_down_part->setCurrentWidget(widgets_[kAnalyzer]);
		});
		connect(ui_.spectrgoram_side_pushbutton, &QPushButton::clicked, [this]() {
			ui_.main_down_part->setCurrentWidget(widgets_[kStaticSpg]);
		});

		connect(ui_.main_down_part, &QStackedWidget::currentChanged, this, [this](int) {
			QWidget* current = ui_.main_down_part->currentWidget();
			for (auto& kv : widgets_) {
				auto dove = std::make_shared<fluctus::DoveParrent>(
					(kv.second == current) ? fluctus::DoveParrent::kActivate : fluctus::DoveParrent::kDeactivate);
				((ArkInterface*)kv.second)->PostDove(dove);
				// отправить dove в kv.second
			}
		});

		QShortcut* saveShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_S), this);
		connect(saveShortcut, &QShortcut::activated, this, &SpectralViewerWindow::RecordSelectionNeed);

		UpdateFFtCombobox(21, 10);
	}
	ui_.main_splitter->setSizes({ 1,1 });
}
void spectral_viewer::SpectralViewerWindow::AddWindow(QWidget * wigdet_ptr, ChartType window_type)
{
	widgets_[window_type] = wigdet_ptr;
	switch (window_type)
	{
	case spectral_viewer::SpectralViewerWindow::kDpxSpectrum:
		ui_.main_up_part->layout()->addWidget(wigdet_ptr);
		qobject_cast<ChartInterface*>(wigdet_ptr)->SetControlButtons(ui_.ctrl_buttons_frame);
		break;
	case spectral_viewer::SpectralViewerWindow::kStaticSpg:
	case spectral_viewer::SpectralViewerWindow::kAnalyzer:
		ui_.main_down_part->addWidget(wigdet_ptr);
		break;
	default:
		break;
	}
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
				ui_.fft_order_combobox->setCurrentIndex(index);  // вызовет emit currentIndexChanged
			}
			emit ui_.fft_order_combobox->currentIndexChanged(index);
		}

	}
}






