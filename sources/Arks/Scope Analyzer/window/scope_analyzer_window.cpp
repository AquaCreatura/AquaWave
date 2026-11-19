#include "scope_analyzer_window.h"

#include "GUI/Charts/ChartInterface.h"
#include "Utilities/parse_tools.h"

#include <qevent.h>

using namespace scope_analyzer;
using namespace analyzer;

namespace
{
	constexpr int kCompactWidth = 600;
}

ScopeAnalyzerWindow::ScopeAnalyzerWindow()
{
    ui_.setupUi(this);
	SetupValueRows();

	ui_.symbol_rate_combobox->addItem("PSK", int(analz_chart_type::kPhasorSpectrum));
	ui_.symbol_rate_combobox->addItem("ENV", int(analz_chart_type::kEnvelopeSpectrum));
	connect(ui_.symbol_rate_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
		ActivateWindow(analz_chart_type(ui_.symbol_rate_combobox->currentData().toInt()));
	});

	// Клик по радиокнопке только переключает график: копирование значения
	// выполняет сам CopyValueLabel.
	connect(ui_.radio_group_chart_type,
		QOverload<int>::of(&QButtonGroup::buttonClicked),
		this,
		[this](int id)
	{
		ActivateWindow(static_cast<analyzer::analz_chart_type>(id));
	});

	chart_values_[analz_chart_type::kPowerSpectrum]		= "0.000000MHz";
	chart_values_[analz_chart_type::kSymbolRateGeneral]	= "0.000kHz";
	chart_values_[analz_chart_type::kAcf]				= "0.000ms";
	chart_values_[analz_chart_type::kConstellation]		= "QPSK";
	RefreshValueDisplays();

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
		UpdateFFtCombobox(21, 12);
	}
	
};

void ScopeAnalyzerWindow::SetupValueRows()
{
	rows_ = {
		{ analz_chart_type::kPowerSpectrum,     "Carrier",    ui_.carrier_radio_button,     ui_.carrier_value_label     },
		{ analz_chart_type::kSymbolRateGeneral, "SymbolRate", ui_.symbol_rate_radio_button, ui_.symbol_rate_value_label },
		{ analz_chart_type::kAcf,               "ACF",        ui_.acf_radio_button,         ui_.acf_value_label         },
		{ analz_chart_type::kConstellation,     "Modulation", ui_.modulation_radio_button,  ui_.modulation_value_label  },
	};

	for (const ValueRow& row : rows_) {
		ui_.radio_group_chart_type->setId(row.radio, (int)row.type);
		row.radio->setText(row.name + ":");
		row.radio->setToolTip(row.name);
	}
}

void ScopeAnalyzerWindow::AddChartWindow(QWidget * widget_ptr, analz_chart_type type_of_chart)
{
	if (charts_.empty()) cur_chart_type_ = type_of_chart;
	charts_[type_of_chart] = widget_ptr;

	switch (type_of_chart)
	{
	case analz_chart_type::undefined:
		break;
	default:
		ui_.harmonic_chart_stacked->addWidget(widget_ptr);
		break;
	}
	update();

}

void ScopeAnalyzerWindow::ActivateWindow(analz_chart_type passed_chart_type)
{
	if (auto casted = dynamic_cast<ChartInterface*>(charts_[cur_chart_type_]))
		casted->ActivateChart(false);

	if (passed_chart_type == analz_chart_type::kSymbolRateGeneral)
		passed_chart_type = analz_chart_type(ui_.symbol_rate_combobox->currentData().toInt());


	if (charts_.count(passed_chart_type) == 0) return;
	//Выставляем нажатие
	{
		analz_chart_type radio_box_type = passed_chart_type;
		if (passed_chart_type == analz_chart_type::kPhasorSpectrum || passed_chart_type == analz_chart_type::kEnvelopeSpectrum)
			radio_box_type = analz_chart_type::kSymbolRateGeneral;

		QAbstractButton* button = ui_.radio_group_chart_type->button((int)radio_box_type);
		button->blockSignals(true);
		button->setChecked(true);
		button->blockSignals(false);	
	}
	ui_.harmonic_chart_stacked->setCurrentWidget(charts_[passed_chart_type]);

	if (auto casted = dynamic_cast<ChartInterface*>(charts_[passed_chart_type])) {
		casted->ActivateChart(true);
		casted->SetControlButtons(ui_.ctrl_buttons_frame);
	}
	cur_chart_type_ = passed_chart_type;
}

analz_chart_type scope_analyzer::ScopeAnalyzerWindow::GetCurrentChart()
{
	return analz_chart_type(ui_.radio_group_chart_type->checkedId());
}

void scope_analyzer::ScopeAnalyzerWindow::SetMaxFFtOrder(int max_fft_order)
{
	const auto cur_fft = ui_.fft_order_combobox->currentData().toInt();
	UpdateFFtCombobox(max_fft_order, cur_fft);
}
void scope_analyzer::ScopeAnalyzerWindow::UpdateHarmonicResult(analz_chart_type type_of_chart, double value, QString modulation)
{
	double divider = 1.0;
	QString suffix;
	int precision = 0;

	switch (type_of_chart)
	{
	case analz_chart_type::kPowerSpectrum:
		divider = 1e6; suffix = "MHz"; precision = 6; break;

	case analz_chart_type::kEnvelopeSpectrum:
	case analz_chart_type::kPhasorSpectrum:
		divider = 1e3; suffix = "kHz"; precision = 3; break;

	case analz_chart_type::kAcf:
		UpdateValueForType(type_of_chart, QString("%1ms").arg(value, 0, 'f', 3));
		return;
	case analz_chart_type::kConstellation:
		UpdateValueForType(type_of_chart, modulation);
		return;

	default:
		return;
	}

	UpdateValueForType(type_of_chart, QString::number(value / divider, 'f', precision) + suffix);
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

void ScopeAnalyzerWindow::resizeEvent(QResizeEvent * event)
{
	QDialog::resizeEvent(event);
	ApplyCompactMode(width() < kCompactWidth);
}

void ScopeAnalyzerWindow::ApplyCompactMode(bool compact)
{
	// Отображение обновляем только при смене режима.
	if (compact_mode_ == compact) return;

	compact_mode_ = compact;
	// В компактном режиме остаются только значения: подписи радиокнопок прячем.
	for (const ValueRow& row : rows_) {
		if (row.radio) row.radio->setText(compact ? QString() : row.name + ":");
	}
}

void ScopeAnalyzerWindow::RefreshValueDisplays()
{
	for (const ValueRow& row : rows_) {
		if (row.value) row.value->SetValue(ValueForType(row.type));
	}
}

void ScopeAnalyzerWindow::UpdateValueForType(analz_chart_type type_of_chart, const QString& value)
{
	chart_values_[DisplayKeyForType(type_of_chart)] = value;
	RefreshValueDisplays();
}

QString ScopeAnalyzerWindow::ValueForType(analz_chart_type type_of_chart) const
{
	const auto found = chart_values_.find(DisplayKeyForType(type_of_chart));
	return (found == chart_values_.end()) ? QString() : found->second;
}

analz_chart_type ScopeAnalyzerWindow::DisplayKeyForType(analz_chart_type type_of_chart)
{
	switch (type_of_chart)
	{
	case analz_chart_type::kPhasorSpectrum:
	case analz_chart_type::kEnvelopeSpectrum:
		return analz_chart_type::kSymbolRateGeneral;
	default:
		return type_of_chart;
	}
}
