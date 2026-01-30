#pragma once
#include "ui_scope_analyzer_window.h"
#include <qdialog.h>
#include <map>


class ScopeAnalyzerWindow : public QDialog
{
Q_OBJECT
public:
	ScopeAnalyzerWindow();

enum chart_type {
	undefined = 0,
	spg = 1,
	spectrum = 2,
	acf = 4,
	phase_spectrum = 8,
	power_spectrum = 16
};
    void SetChartWindow(QWidget* wigdet_ptr, chart_type type_of_chart);
protected:
    Ui::ScopeAnalyzerWindow ui_;
	std::map<chart_type, QWidget*> charts_;
};

