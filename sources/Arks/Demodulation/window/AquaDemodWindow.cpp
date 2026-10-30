#include "AquaDemodWindow.h"

#include <qlayout.h>

AquaDemodWindow::AquaDemodWindow()
{
	ui_.setupUi(this);

	constel_chart_ = new constel::ChartConstel(ui_.frame_3);
	{
		auto* constel_layout = new QVBoxLayout(ui_.frame_3);
		constel_layout->setContentsMargins(0, 0, 0, 0);
		constel_layout->addWidget(constel_chart_);
	}

	settings_delayed_ = std::make_unique<utility_aqua::DelayedCaller>(300);

	SetupModulations();
	SetupSliders();
	SetupRanges();
}

AquaDemodWindow::~AquaDemodWindow() = default;

void AquaDemodWindow::SetupModulations()
{
	// Список согласован с CalculateModulationPivots (PSK/QAM, включая cross-QAM 32/128).
	const QStringList modulations = { "QPSK", "BPSK", "8PSK", "16QAM", "32QAM", "64QAM", "128QAM", "256QAM" };
	{
		QSignalBlocker blocker(ui_.modulation_combobox);
		for (const QString& modulation : modulations)
			ui_.modulation_combobox->addItem(modulation, modulation);
		ui_.modulation_combobox->setCurrentText("QPSK");
	}

	connect(ui_.modulation_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, [this](int) { emit DemodSettingsChanged(); });
}

void AquaDemodWindow::SetupSliders()
{
	// Ползунки задают «скорость» соответствующей петли в процентах.
	for (QSlider* slider : { ui_.pll_slider, ui_.tll_slider, ui_.eq_slider }) {
		slider->setRange(0, 100);
		slider->setValue(50);
	}

	connect(ui_.pll_slider, &QSlider::valueChanged, this, [this](int value) { emit PllSpeedChanged(value / 100.0); });
	connect(ui_.tll_slider, &QSlider::valueChanged, this, [this](int value) { emit TllSpeedChanged(value / 100.0); });
	connect(ui_.eq_slider, &QSlider::valueChanged, this, [this](int value) { emit EqSpeedChanged(value / 100.0); });
}

void AquaDemodWindow::SetupRanges()
{
	ui_.snr_spinbox->setMinimum(-30.0);

	// Ручной ввод частоты перестраивает тракт, поэтому изменения сглаживаются.
	auto request_reload = [this](double) {
		settings_delayed_->Call([this]() { emit DemodSettingsChanged(); });
	};
	connect(ui_.fc_mhz_spinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, request_reload);
	connect(ui_.bd_khz_spinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, request_reload);
}

void AquaDemodWindow::SetFreqParams(aqua_opt<double> carrier_hz, aqua_opt<double> symbol_rate_hz)
{
	QSignalBlocker fc_blocker(ui_.fc_mhz_spinbox);
	QSignalBlocker bd_blocker(ui_.bd_khz_spinbox);
	if (carrier_hz) {
		ui_.fc_mhz_spinbox->setValue(*carrier_hz / 1.e6);
	}
	if (symbol_rate_hz) {
		ui_.bd_khz_spinbox->setValue(*symbol_rate_hz / 1.e3);
	}
}

QString AquaDemodWindow::GetModulation() const
{
	return ui_.modulation_combobox->currentData().toString();
}

double AquaDemodWindow::GetCarrierHz() const
{
	return ui_.fc_mhz_spinbox->value() * 1.e6;
}

double AquaDemodWindow::GetSymbolRateHz() const
{
	return ui_.bd_khz_spinbox->value() * 1.e3;
}

double AquaDemodWindow::GetPllSpeed() const
{
	return ui_.pll_slider->value() / 100.0;
}

double AquaDemodWindow::GetTllSpeed() const
{
	return ui_.tll_slider->value() / 100.0;
}

double AquaDemodWindow::GetEqSpeed() const
{
	return ui_.eq_slider->value() / 100.0;
}

void AquaDemodWindow::PushConstelData(std::vector<Ipp32fc>& constel_data)
{
	if (constel_chart_)
		constel_chart_->PushData(constel_data);
}

void AquaDemodWindow::ClearConstelData()
{
	if (constel_chart_)
		constel_chart_->ClearData();
}

void AquaDemodWindow::UpdateEstimates(double snr_db, double carrier_offset_hz, double symbol_rate_hz)
{
	ui_.snr_spinbox->setValue(snr_db);
	ui_.fc_khz_est_spinbox->setValue(carrier_offset_hz / 1.e3);
	ui_.bd_khz_est_spinbox->setValue(symbol_rate_hz / 1.e3);
}
