#pragma once
#include <Arks/Interfaces/special_defs/analyzer_defs.h>
#include "GUI/Tools/CopyValueLabel.h"
#include "Utilities/qt_utility.h"

#include <qdialog.h>
#include "ui_scope_analyzer_window.h"

#include <map>
#include <vector>

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
	void UpdateHarmonicResult(analz_chart_type type_of_chart, double value, QString modulation = "");
	void UpdateFFtCombobox(const int max_order, const int cur_fft_order);
signals:
	void FftChangeNeed(int new_fft);

protected:
	void resizeEvent(QResizeEvent* event) override;

	// Одно отображаемое значение: радиокнопка и поле значения.
	struct ValueRow {
		analz_chart_type type = analz_chart_type::undefined;
		QString          name;  // подпись радиокнопки, например "Carrier"
		QRadioButton*    radio = nullptr;
		CopyValueLabel*  value = nullptr;
	};

	// Наполняет rows_ по элементам ui_.
	void SetupValueRows();
	// В компактном режиме прячет подписи радиокнопок, оставляя только значения.
	void ApplyCompactMode(bool compact);
	void RefreshValueDisplays();
	void UpdateValueForType(analz_chart_type type_of_chart, const QString& value);
	QString ValueForType(analz_chart_type type_of_chart) const;
	// Ключ, под которым значение хранится (символьная скорость для PSK и AM —
	// это одно и то же отображаемое значение).
	static analz_chart_type DisplayKeyForType(analz_chart_type type_of_chart);

protected:
	Ui::ScopeAnalyzerWindow ui_;
	std::unique_ptr<utility_aqua::DelayedCaller> fft_delayed_;
	std::map<analz_chart_type, QWidget*> charts_;
	analz_chart_type cur_chart_type_ = analz_chart_type::undefined;
	std::vector<ValueRow> rows_;
	std::map<analz_chart_type, QString> chart_values_;
	bool compact_mode_{ false };
};

}
