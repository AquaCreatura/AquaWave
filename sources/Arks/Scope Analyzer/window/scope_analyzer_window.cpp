#include "scope_analyzer_window.h"
#include "Tools\parse_tools.h"
ScopeAnalyzerWindow::ScopeAnalyzerWindow()
{
    ui_.setupUi(this);
	
};

void ScopeAnalyzerWindow::SetChartWindow(QWidget * widget_ptr, chart_type type_of_chart)
{
	charts_[type_of_chart] = widget_ptr;

	if (type_of_chart == chart_type::spg) {
		ui_.spg_chart->layout()->addWidget(widget_ptr);
	}

	if (type_of_chart == chart_type::spectrum) {
		ui_.dpx_spectrum_chart->layout()->addWidget(widget_ptr);
	}
	//ui_.DpxChart->layout()->addWidget(widget_ptr);
}

