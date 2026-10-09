#pragma once
#include "ui_scope_analyzer_window.h"
#include "Utilities/qt_utility.h"
#include <qdialog.h>
#include <map>
#include <Arks/Interfaces/special_defs/analyzer_defs.h>
using namespace analyzer;
namespace scope_analyzer {

class ScopeAnalyzerWindow : public QDialog
{
	Q_OBJECT
public:
	ScopeAnalyzerWindow();


	void AddChartWindow(QWidget* wigdet_ptr, analz_chart_type type_of_chart);
	void ActivateWindow(analz_chart_type type_of_chart);
	analz_chart_type GetCurrentChart();
	void SetMaxFFtOrder(int max_fft_order);
	void UpdateHarmonicResult(analz_chart_type type_of_chart, double value, QString modulation);
	void UpdateFFtCombobox(const int max_order, const int cur_fft_order);
signals:
	void FftChangeNeed(int new_fft);

protected:
	Ui::ScopeAnalyzerWindow ui_;
	std::unique_ptr<utility_aqua::DelayedCaller> fft_delayed_;
	std::map<analz_chart_type, QWidget*> charts_;
	analz_chart_type cur_chart_type_;
};

}