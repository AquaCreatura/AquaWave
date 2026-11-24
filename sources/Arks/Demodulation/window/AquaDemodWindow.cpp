#include "AquaDemodWindow.h"

#include "DSP Tools/Modem/ModulationPoints.h"

#include <qformlayout.h>
#include <qlabel.h>
#include <qlayout.h>

#include <algorithm>
#include <cmath>

namespace
{
	const char* const kButtonImageBase = ":/buttons/button_images/";
	// Размер кнопок Start/Reset: в обычном режиме как раньше, в компактном — меньше.
	constexpr int kFullButtonSize = 50;
	constexpr int kCompactButtonSize = 26;
	// Порог, ниже которого отдельное окно демодулятора переходит в компактный вид.
	constexpr int kCompactHeight = 350;

	QString ButtonImage(const char* file_name)
	{
		return QString(kButtonImageBase) + file_name;
	}
}

AquaDemodWindow::AquaDemodWindow(bool is_light_mode) :
	is_light_mode_(is_light_mode)
{
	ui_.setupUi(this);
	settings_delayed_ = std::make_unique<utility_aqua::DelayedCaller>(300);

	SetupConstellationChart();
	SetupButtons();
	SetupModulations();
	SetupEqAlgorithms();
	SetupSliders();
	SetupRanges();

	connect(ui_.run_button, &QPushButton::clicked, this, &AquaDemodWindow::OnStartStopClicked);
	connect(ui_.reset_button, &QPushButton::clicked, this, &AquaDemodWindow::OnResetClicked);

	if (is_light_mode_)
		SetupLightLayout();

	// Встроенное окно всегда компактное, отдельное переключается по размеру.
	ApplyCompactMode(is_light_mode_);
}

AquaDemodWindow::~AquaDemodWindow() = default;

// --- Полная раскладка (созвездие, параметры, петли) ---

void AquaDemodWindow::SetupConstellationChart()
{
	constel_chart_ = new constel::ChartConstel(ui_.constel_box);
	auto* constel_layout = new QVBoxLayout(ui_.constel_box);
	constel_layout->setContentsMargins(0, 0, 0, 0);
	constel_layout->addWidget(constel_chart_);
}

void AquaDemodWindow::SetupButtons()
{
	ui_.run_button->setText("");
	ui_.run_button->setCheckable(true);
	ui_.reset_button->setText("");

	// Размеры кнопок в обычном виде берём из формы, чтобы вернуть их
	// при выходе из компактного режима.
	full_button_min_ = ui_.run_button->minimumSize();
	full_button_max_ = ui_.run_button->maximumSize();

	ApplyButtonSizes(false);
}

void AquaDemodWindow::ApplyButtonSizes(bool compact)
{
	const int size = compact ? kCompactButtonSize : kFullButtonSize;
	const QString start = ButtonImage("start_water_button.png");
	const QString stop = ButtonImage("stop_water_button.png");
	const QString refresh = ButtonImage("refresh_water_button.png");

	ui_.run_button->setStyleSheet(buildButtonStyleSheet(size, stop, stop, start, start, start));
	ui_.reset_button->setStyleSheet(buildButtonStyleSheet(size, refresh, refresh, refresh, refresh, refresh));

	if (compact) {
		ui_.run_button->setFixedSize(size, size);
		ui_.reset_button->setFixedSize(size, size);
	}
	else {
		ui_.run_button->setMinimumSize(full_button_min_);
		ui_.run_button->setMaximumSize(full_button_max_);
		ui_.reset_button->setMinimumSize(full_button_min_);
		ui_.reset_button->setMaximumSize(full_button_max_);
	}
}

void AquaDemodWindow::OnStartStopClicked()
{
	is_running_ = !is_running_;
	UpdateRunUI();
	emit RunToggled(is_running_);
}

void AquaDemodWindow::OnResetClicked()
{
	emit ResetRequested();
}

void AquaDemodWindow::SetupModulations()
{
	// Список согласован с CalculateModulationPivots (PSK/QAM, включая cross-QAM 32/128/512).
	const QStringList modulations = { "QPSK", "BPSK", "8PSK", "16QAM", "32QAM", "64QAM", "128QAM", "256QAM", "512QAM", "1024QAM" };
	{
		QSignalBlocker blocker(ui_.mod_combo);
		for (const QString& modulation : modulations)
			ui_.mod_combo->addItem(modulation, modulation);
		ui_.mod_combo->setCurrentText("QPSK");
	}

	connect(ui_.mod_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, [this](int) {
			// Смена модуляции — ставим её оптимальные скорости петель,
			// затем просим пересобрать тракт.
			ApplyModulationDefaults();
			emit DemodSettingsChanged();
			emit ModulationChanged(GetModulation());
		});
}

void AquaDemodWindow::SetLoopValue(Loop& loop, double value)
{
	if (!loop.slider)
		return;
	// Через ползунок: сработает on_slider_changed, обновит спинабокс и
	// эмитит соответствующий *SpeedChanged, применяя значение в демодуляторе.
	loop.slider->setValue(SliderValue(loop.slider, value));
}

void AquaDemodWindow::SetEqAlgorithm(const char* algorithm)
{
	if (!algorithm)
		return;
	const int index = ui_.eq_alg_combo->findText(QString::fromLatin1(algorithm));
	if (index >= 0)
		ui_.eq_alg_combo->setCurrentIndex(index);
}

void AquaDemodWindow::ApplyModulationDefaults()
{
	const aq_demod::DmdSliderStructure setup =
		aq_demod::GetDefaultSliderSetup(GetModulation().toUtf8().constData());
	SetLoopValue(pll_loop_, setup.pll);
	SetLoopValue(tll_loop_, setup.tll);
	SetLoopValue(eq_loop_, setup.eq_koeff);
	SetEqAlgorithm(setup.eq_alg);
}

void AquaDemodWindow::SetupEqAlgorithms()
{
	// Список алгоритмов задан в .ui (AUTO/MMA/CMA/LMS); AUTO — дефолт.
	// Дублируем его в коде только если .ui пуст, и всегда заполняем userData,
	// чтобы GetEqAlgorithm()/SetEqAlgorithm() работали по имени.
	const QStringList algorithms = { "AUTO", "MMA", "CMA", "LMS" };
	{
		QSignalBlocker blocker(ui_.eq_alg_combo);
		if (ui_.eq_alg_combo->count() == 0)
			for (const QString& algorithm : algorithms)
				ui_.eq_alg_combo->addItem(algorithm);
		for (int i = 0; i < ui_.eq_alg_combo->count(); ++i)
			ui_.eq_alg_combo->setItemData(i, ui_.eq_alg_combo->itemText(i));
		ui_.eq_alg_combo->setCurrentText("AUTO");
	}

	// Смена алгоритма применяется на лету, пересборка тракта не нужна.
	connect(ui_.eq_alg_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, [this](int) { emit EqAlgorithmChanged(GetEqAlgorithm()); });
}

void AquaDemodWindow::SetupSliders()
{
	// Группы петель (label, слайдер, спинабокс); границы и подключение
	// ставятся в SetSpeedRanges() из границ, возвращаемых демодулятором.
	pll_loop_ = { ui_.pll_label, ui_.pll_slider, ui_.pll_spin };
	tll_loop_ = { ui_.tll_label, ui_.tll_slider, ui_.tll_spin };
	eq_loop_  = { ui_.eq_label, ui_.eq_slider, ui_.eq_spin };
}

void AquaDemodWindow::SetupRanges()
{
	ui_.snr_est->setMinimum(-30.0);

	// Ручной ввод частоты перестраивает тракт, поэтому изменения сглаживаются.
	auto request_reload = [this](double) {
		settings_delayed_->Call([this]() { emit DemodSettingsChanged(); });
	};
	connect(ui_.fc_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, request_reload);
	connect(ui_.sr_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, request_reload);
}

// --- Компактная (светлая) раскладка встроенного окна: созвездие, SNR,
// модуляция, Start/Stop — всегда видны; несущая и символьная скорость
// приходят из анализатора. Раскладка применяется один раз и не снимается.
void AquaDemodWindow::SetupLightLayout()
{
	light_frame_ = new QFrame(this);
	auto* root = new QVBoxLayout(light_frame_);
	root->setContentsMargins(0, 0, 0, 0);
	root->setSpacing(4);

	light_params_frame_ = new QFrame(light_frame_);
	light_params_grid_ = new QGridLayout(light_params_frame_);
	light_params_grid_->setContentsMargins(4, 2, 4, 2);
	light_params_grid_->setHorizontalSpacing(6);
	light_params_grid_->setVerticalSpacing(2);
	light_params_grid_->addWidget(new QLabel(tr("Carrier:"), light_params_frame_), 0, 0);
	light_params_grid_->addWidget(new QLabel(tr("Symbol rate:"), light_params_frame_), 1, 0);
	light_params_grid_->addWidget(ui_.fc_spin, 0, 1);
	light_params_grid_->addWidget(ui_.sr_spin, 1, 1);
	light_params_frame_->setVisible(!is_light_mode_);

	auto* body = new QHBoxLayout();
	body->setContentsMargins(0, 0, 0, 0);
	body->setSpacing(6);

	auto* controls = new QVBoxLayout();
	controls->setContentsMargins(0, 0, 0, 0);
	controls->setSpacing(4);

	light_values_grid_ = new QGridLayout();
	light_values_grid_->setContentsMargins(0, 0, 0, 0);
	light_values_grid_->setHorizontalSpacing(6);
	light_values_grid_->setVerticalSpacing(2);
	light_values_grid_->addWidget(new QLabel(tr("SNR:"), light_frame_), 0, 0);
	light_values_grid_->addWidget(new QLabel(tr("Mod:"), light_frame_), 1, 0);
	light_values_grid_->addWidget(ui_.snr_est, 0, 1);
	light_values_grid_->addWidget(ui_.mod_combo, 1, 1);

	light_buttons_ = new QHBoxLayout();
	light_buttons_->setContentsMargins(0, 0, 0, 0);
	light_buttons_->setSpacing(4);
	light_buttons_->addStretch();
	light_buttons_->insertWidget(0, ui_.reset_button);
	light_buttons_->insertWidget(1, ui_.run_button);

	controls->addLayout(light_values_grid_);
	controls->addLayout(light_buttons_);
	controls->addStretch();
	body->addLayout(controls, 0);
	body->addWidget(constel_chart_, 1);
	light_body_ = body;

	root->addWidget(light_params_frame_);
	root->addLayout(body, 1);
	light_frame_->setVisible(false);
	ui_.root_grid->addWidget(light_frame_, 0, 0, 2, 2);
}

// Переключение между полной и компактной раскладками:
// - встроенное (is_light_mode_): светлая раскладка, одна сторона, неизменяема;
// - отдельное: компактный режим скрывает только pll_frame, размеры кнопок
//   уменьшаются.
void AquaDemodWindow::ApplyCompactMode(bool compact)
{
	if (compact_active_ == compact)
		return;
	compact_active_ = compact;

	ApplyButtonSizes(compact);

	if (is_light_mode_) {
		// Встроенное окно: без отступов, чтобы при встраивании
		// не было рамки в рамке.
		ui_.root_grid->setContentsMargins(0, 0, 0, 0);
		light_frame_->setVisible(true);
		ui_.params_frame->setVisible(false);
		ui_.constel_frame->setVisible(false);
		return;
	}
}


// --- Внешнее API: параметры и оценки ---

void AquaDemodWindow::SetFreqParams(aqua_opt<double> carrier_hz, aqua_opt<double> symbol_rate_hz)
{
	QSignalBlocker fc_blocker(ui_.fc_spin);
	QSignalBlocker bd_blocker(ui_.sr_spin);
	if (carrier_hz) {
		ui_.fc_spin->setValue(*carrier_hz / 1.e6);
	}
	if (symbol_rate_hz) {
		ui_.sr_spin->setValue(*symbol_rate_hz / 1.e3);
	}
}

QString AquaDemodWindow::GetModulation() const
{
	return ui_.mod_combo->currentData().toString();
}

QString AquaDemodWindow::GetEqAlgorithm() const
{
	return ui_.eq_alg_combo->currentData().toString();
}

double AquaDemodWindow::GetCarrierHz() const
{
	return ui_.fc_spin->value() * 1.e6;
}

double AquaDemodWindow::GetSymbolRateHz() const
{
	return ui_.sr_spin->value() * 1.e3;
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

	// Сразу выставляем оптимальные для текущей модуляции значения.
	ApplyModulationDefaults();
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
	ui_.run_button->setChecked(is_running_);
	ui_.fc_spin->setEnabled(!is_running_);
	ui_.sr_spin->setEnabled(!is_running_);
	ui_.mod_combo->setEnabled(!is_running_);
}

void AquaDemodWindow::SetRunningState(bool running)
{
	if (is_running_ == running)
		return;
	is_running_ = running;
	UpdateRunUI();
}

// --- Созвездие ---

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

void AquaDemodWindow::SetConstelPivots(const std::vector<Ipp32fc>& pivots)
{
	if (constel_chart_)
		constel_chart_->SetPivots(pivots);
}

void AquaDemodWindow::ClearConstelPivots()
{
	if (constel_chart_)
		constel_chart_->ClearPivots();
}

void AquaDemodWindow::UpdateEstimates(double snr_db, double carrier_offset_hz, double symbol_rate_hz)
{
	ui_.snr_est->setValue(snr_db);
	ui_.fc_est->setValue(carrier_offset_hz / 1.e3);
	ui_.sr_est->setValue(symbol_rate_hz / 1.e3);
}
