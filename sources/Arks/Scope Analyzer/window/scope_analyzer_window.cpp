#include "scope_analyzer_window.h"
#include "Tools\parse_tools.h"

using namespace scope_analyzer;
ScopeAnalyzerWindow::ScopeAnalyzerWindow()
{
    ui_.setupUi(this);

	ui_.radio_group_chart_type->setId(ui_.carrier_radio_button, scope_chart_type::kPowerSpectrum);
	ui_.radio_group_chart_type->setId(ui_.symbol_rate_radio_button, scope_chart_type::kPhasorSpectrum);
	ui_.radio_group_chart_type->setId(ui_.bandwidth_radio_button, scope_chart_type::kBandwidth);
	ui_.radio_group_chart_type->setId(ui_.acf_radio_button, scope_chart_type::kAcf);

	connect(ui_.radio_group_chart_type,
		QOverload<int>::of(&QButtonGroup::buttonClicked),
		this,
		[this](int id)
	{
		ActivateWindow(static_cast<scope_chart_type>(id));
	});
	
};

void ScopeAnalyzerWindow::AddChartWindow(QWidget * widget_ptr, scope_chart_type type_of_chart)
{
	charts_[type_of_chart] = widget_ptr;

	switch (type_of_chart)
	{
	case scope_chart_type::undefined:
		break;
	case scope_chart_type::kBaseSpg:
		ui_.spg_chart->layout()->addWidget(widget_ptr);
		break;
	case scope_chart_type::kBaseSpectrum:
		ui_.dpx_spectrum_chart->layout()->addWidget(widget_ptr);
		break;
	default:
		ui_.harmonic_chart_stacked->addWidget(widget_ptr);
		break;
	}

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

