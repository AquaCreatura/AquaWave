#pragma once
#include "ui_scope_analyzer_window.h"
#include "Tools/qt_utility.h"
#include <qdialog.h>
#include <map>
namespace scope_analyzer {

enum scope_chart_type {
	undefined		= 0,
	kAcf			= 1, //АКФ

	kPhasorSpectrum = 2, //Спектр мгновенной частоты (Символьная для PSK)
	kEnvelopeSpectrum = 4, //Спектр огибающей (Символьная для AM)

	kPowerSpectrum  = 8, //Спектр степени (Отстройка несущей)
	
	kBandwidth      = 16, //Ширина полосы
	
	kConstellation  = 32
};


class ScopeAnalyzerWindow : public QDialog
{
	Q_OBJECT
public:
	ScopeAnalyzerWindow();


	void AddChartWindow(QWidget* wigdet_ptr, scope_chart_type type_of_chart);
	void ActivateWindow(scope_chart_type type_of_chart);
	scope_chart_type GetCurrentChart();
	void SetMaxFFtOrder(int max_fft_order);

	void UpdateFFtCombobox(const int max_order, const int cur_fft_order);
signals:
	void FftChangeNeed(int new_fft);

protected:
	Ui::ScopeAnalyzerWindow ui_;
	std::unique_ptr<utility_aqua::DelayedCaller> fft_delayed_;
	std::map<scope_chart_type, QWidget*> charts_;
	scope_chart_type cur_chart_type_;
};

}