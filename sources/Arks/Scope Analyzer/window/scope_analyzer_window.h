#pragma once
#include "ui_scope_analyzer_window.h"
#include <qdialog.h>
#include <map>
namespace scope_analyzer {

enum scope_chart_type {
	undefined		= 0,
	kBaseSpg		= 1,
	kBaseSpectrum	= 2,

	kAcf			= 4, //АКФ

	kPhasorSpectrum = 8, //Спектр мгновенной частоты (Символьная для PSK)
	kEnvelopeSpectrum = 16, //Спектр огибающей (Символьная для AM)

	kPowerSpectrum  = 32, //Спектр степени (Отстройка несущей)
	
	kBandwidth      = 64, //Ширина полосы
};


class ScopeAnalyzerWindow : public QDialog
{
	Q_OBJECT
public:
	ScopeAnalyzerWindow();


	void AddChartWindow(QWidget* wigdet_ptr, scope_chart_type type_of_chart);
	void ActivateWindow(scope_chart_type type_of_chart);
protected:
	Ui::ScopeAnalyzerWindow ui_;
	std::map<scope_chart_type, QWidget*> charts_;
};

}