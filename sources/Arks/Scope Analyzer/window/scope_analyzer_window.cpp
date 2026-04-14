#include "scope_analyzer_window.h"
#include "Tools/parse_tools.h"
using namespace scope_analyzer;
ScopeAnalyzerWindow::ScopeAnalyzerWindow()
{
    ui_.setupUi(this);

	ui_.radio_group_chart_type->setId(ui_.carrier_radio_button		  , scope_chart_type::kPowerSpectrum	);
	ui_.radio_group_chart_type->setId(ui_.symbol_rate_psk_radio_button, scope_chart_type::kPhasorSpectrum	);
	ui_.radio_group_chart_type->setId(ui_.symbol_rate_am_radio_button , scope_chart_type::kEnvelopeSpectrum	);
	ui_.radio_group_chart_type->setId(ui_.bandwidth_radio_button	  , scope_chart_type::kBandwidth		);
	ui_.radio_group_chart_type->setId(ui_.acf_radio_button			  , scope_chart_type::kAcf				);

	connect(ui_.radio_group_chart_type,
		QOverload<int>::of(&QButtonGroup::buttonClicked),
		this,
		[this](int id)
	{
		ActivateWindow(static_cast<scope_chart_type>(id));
	});
	fft_delayed_ = std::make_unique<utility_aqua::DelayedCaller>(500);
	//Определяем Combobox для FFT
	{
		connect(ui_.fft_order_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
			fft_delayed_->Call([&]() {
					int fft_id = ui_.fft_order_combobox->currentData().toInt();
					emit FftChangeNeed(fft_id);
				}
			);
			
		});
		UpdateFFtCombobox(21, 15);
	}
	
};

void ScopeAnalyzerWindow::AddChartWindow(QWidget * widget_ptr, scope_chart_type type_of_chart)
{
	charts_[type_of_chart] = widget_ptr;

	switch (type_of_chart)
	{
	case scope_chart_type::undefined:
		break;
	case scope_chart_type::kConstellation:
		ui_.constell_chart->layout()->addWidget(widget_ptr);
		break;
	default:
		ui_.harmonic_chart_stacked->addWidget(widget_ptr);
		break;
	}
	update();

}

void ScopeAnalyzerWindow::ActivateWindow(scope_chart_type type_of_chart)
{
	if (charts_.count(type_of_chart) == 0 || type_of_chart <= scope_chart_type::kBaseSpectrum) return;
	//Выставляем нажатие
	{
		QAbstractButton* button = ui_.radio_group_chart_type->button(type_of_chart);
		button->blockSignals(true);
		button->setChecked(true);
		button->blockSignals(false);	
	}
	ui_.harmonic_chart_stacked->setCurrentWidget(charts_[type_of_chart]);
}

void scope_analyzer::ScopeAnalyzerWindow::SetMaxFFtOrder(int max_fft_order)
{
	const auto cur_fft = ui_.fft_order_combobox->currentData().toInt();
	UpdateFFtCombobox(max_fft_order, cur_fft);
}

void scope_analyzer::ScopeAnalyzerWindow::UpdateFFtCombobox(const int max_order, const int cur_fft_order)
{
	{
		QSignalBlocker blocker(ui_.fft_order_combobox);
		ui_.fft_order_combobox->clear();
		const int min_counter = qBound(4, max_order - 2, 8);
		for (int fft_counter = 8; fft_counter <= max_order; fft_counter++) {
			QString item_text = QString("%1").arg(aqua_parse_tools::ValueToString(1 << fft_counter, 0, " ").c_str());
			ui_.fft_order_combobox->addItem(item_text, fft_counter);
		}
	}
	{
		int target_fft = cur_fft_order;
		int index = ui_.fft_order_combobox->findData(target_fft);
		if (index != -1) {
			ui_.fft_order_combobox->setCurrentIndex(index);  // вызовет emit currentIndexChanged
		}
		else
		{
			ui_.fft_order_combobox->setCurrentIndex(ui_.fft_order_combobox->count() / 2);
		}
	}
}

