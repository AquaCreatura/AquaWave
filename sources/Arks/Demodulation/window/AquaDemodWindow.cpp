#include "AquaDemodWindow.h"

#include <qlayout.h>

#include <algorithm>
#include <cmath>

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

	connect(ui_.start_stop_button, &QPushButton::clicked, this, [this]() {
		is_running_ = !is_running_;
		UpdateRunUI();
		emit RunToggled(is_running_);
	});
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
	// Группы петель (label, слайдер, спинабокс); границы и подключение
	// ставятся в SetSpeedRanges() из границ, возвращаемых демодулятором.
	pll_loop_ = { ui_.label_5, ui_.pll_slider, ui_.pll_spinbox };
	tll_loop_ = { ui_.label_8, ui_.tll_slider, ui_.tll_spinbox };
	eq_loop_ = { ui_.label_9, ui_.eq_slider, ui_.eq_spinbox };
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
	return SliderSpeed(ui_.pll_slider);
}

double AquaDemodWindow::GetTllSpeed() const
{
	return SliderSpeed(ui_.tll_slider);
}

double AquaDemodWindow::GetEqSpeed() const
{
	return SliderSpeed(ui_.eq_slider);
}

void AquaDemodWindow::SetSpeedRanges(aqua::Limits<double> pll, aqua::Limits<double> tll,
                                     aqua::Limits<double> eq,
                                     double pll_default, double tll_default, double eq_default)
{
	pll_loop_.limits = pll;
	tll_loop_.limits = tll;
	eq_loop_.limits = eq;

	SetupLoop(pll_loop_, pll_default, [this](double value) { emit PllSpeedChanged(value); });
	SetupLoop(tll_loop_, tll_default, [this](double value) { emit TllSpeedChanged(value); });
	SetupLoop(eq_loop_, eq_default, [this](double value) { emit EqSpeedChanged(value); });
}

int AquaDemodWindow::SliderValue(QSlider* slider, double speed) const
{
	const SliderInfo& info = SliderParams(slider);
	const double ratio = info.Limits.pos(speed);
	return std::clamp(static_cast<int>(std::lround(ratio * info.Steps)), 0, info.Steps);
}

double AquaDemodWindow::SliderSpeed(QSlider* slider) const
{
	const SliderInfo& info = SliderParams(slider);
	return info.Limits.lerp(static_cast<double>(slider->value()) / static_cast<double>(info.Steps));
}

void AquaDemodWindow::SetupLoop(Loop& loop, double default_value, const std::function<void(double)>& on_speed)
{
	QLabel* label = loop.label;
	QSlider* slider = loop.slider;
	QDoubleSpinBox* spin = loop.spin;
	const aqua::Limits<double>& limits = loop.limits;

	const bool is_tll = (slider == ui_.tll_slider);
	const double step = is_tll ? 1e-4 : 1e-5;
	// Шаг ползунка выбран так, чтобы диапазон соответствовал реальным
	// границам параметра петли с разумным числом позиций.
	const int steps = static_cast<int>(std::round(limits.delta() / step));
	loop.steps = steps;

	spin->setMinimum(limits.low);
	spin->setMaximum(limits.high);
	spin->setDecimals(is_tll ? 4 : 5);
	spin->setSingleStep(step);

	QSignalBlocker slider_blocker(slider);
	QSignalBlocker spin_blocker(spin);
	slider->setRange(0, steps);
	slider->setValue(SliderValue(slider, default_value));
	spin->setValue(default_value);

	QString name = is_tll ? "TLL" : (slider == ui_.pll_slider ? "PLL" : "Equaliser");
	label->setText(name + " [" + FormatRangeEnd(limits.low) + " .. " + FormatRangeEnd(limits.high) + "]");

	const auto on_slider_changed = [this, on_speed, slider, spin](int)
	{
		const double value = SliderSpeed(slider);
		QSignalBlocker spin_blocker(spin);
		on_speed(value);
		spin->setValue(value);
	};
	const auto on_spin_changed = [this, slider](double value)
	{
		slider->setValue(SliderValue(slider, value));
	};
	connect(slider, &QSlider::valueChanged, this, on_slider_changed);
	connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, on_spin_changed);
}

AquaDemodWindow::SliderInfo AquaDemodWindow::SliderParams(QSlider* slider) const
{
	if (slider == ui_.pll_slider) {
		return SliderInfo{ pll_loop_.limits, pll_loop_.steps };
	}
	if (slider == ui_.tll_slider) {
		return SliderInfo{ tll_loop_.limits, tll_loop_.steps };
	}
	return SliderInfo{ eq_loop_.limits, eq_loop_.steps };
}

QString AquaDemodWindow::FormatRangeEnd(double value)
{
	QString s = QString::number(value, 'F', 4);
	while (s.length() >= 3 && s.at(s.length() - 1) == '0' && s.at(s.length() - 2) != '.')
		s.chop(1);
	return s;
}

void AquaDemodWindow::UpdateRunUI()
{
	ui_.start_stop_button->setText(is_running_ ? "Stop" : "Start");
	ui_.fc_mhz_spinbox->setEnabled(!is_running_);
	ui_.bd_khz_spinbox->setEnabled(!is_running_);
}

void AquaDemodWindow::SetRunningState(bool running)
{
	if (is_running_ == running)
		return;
	is_running_ = running;
	UpdateRunUI();
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
