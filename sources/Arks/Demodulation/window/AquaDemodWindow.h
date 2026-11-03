#pragma once
#include "Utilities/utility_aqua.h"
#include "Utilities/qt_utility.h"
#include "GUI/Charts/Constel/ConstelChart.h"

#include "aqua_defines.h"

#include <qdialog.h>
#include <qpointer.h>
#include <qslider.h>
#include "ui_AquaDemodWindow.h"

#include <functional>
#include <memory>

using namespace utility_aqua;

class AquaDemodWindow : public QDialog
{
	Q_OBJECT
public:
	AquaDemodWindow();
	~AquaDemodWindow();

	// Заполняет поля частот значениями извне (например, из гармонического анализатора).
	void SetFreqParams(aqua_opt<double> carrier_hz, aqua_opt<double> symbol_rate_hz);

	// Реальные границы и дефолты петель из демодулятора; настройка ползунков и подписей.
	void SetSpeedRanges(aqua::Limits<double> pll, aqua::Limits<double> tll,
						aqua::Limits<double> eq,
						double pll_default, double tll_default, double eq_default);

	// Включение/выключение демодуляции: блокирует ввод несущей и скорости,
	// меняет текст кнопки Start/Stop.
	void SetRunningState(bool running);

	QString	GetModulation() const;
	double	GetCarrierHz() const;
	double	GetSymbolRateHz() const;
	double	GetPllSpeed() const;
	double	GetTllSpeed() const;
	double	GetEqSpeed() const;

	// Отрисовка созвездия и обновление оценочных значений.
	void PushConstelData(std::vector<Ipp32fc>& constel_data);
	void ClearConstelData();
	void UpdateEstimates(double snr_db, double carrier_offset_hz, double symbol_rate_hz);

signals:
	// Изменились «тяжёлые» параметры демодуляции — требуется пересборка тракта.
	void DemodSettingsChanged();
	// Скорости в единицах реальных параметров петель (см. SetSpeedRanges).
	void PllSpeedChanged(double speed);
	void TllSpeedChanged(double speed);
	void EqSpeedChanged(double speed);
	void RunToggled(bool running);

protected:
	void SetupModulations();
	void SetupSliders();
	void SetupRanges();

private:
	struct SliderInfo
	{
		aqua::Limits<double> Limits;
		int Steps = 1;
	};

	// Группа петель: label, слайдер и спинабокс — один параметр.
	struct Loop
	{
		QLabel*       label = nullptr;
		QSlider*      slider = nullptr;
		QDoubleSpinBox* spin = nullptr;
		aqua::Limits<double> limits{ 0.0 };
		int steps = 1;
	};

	Loop pll_loop_, tll_loop_, eq_loop_;

	void SetupLoop(Loop& loop, double default_value, const std::function<void(double)>& on_speed);
	SliderInfo SliderParams(QSlider* slider) const;
	int SliderValue(QSlider* slider, double speed) const;
	double SliderSpeed(QSlider* slider) const;
	QString FormatRangeEnd(double value);
	void UpdateRunUI();

	Ui::AquaDemodWindow								ui_;
	QPointer<constel::ChartConstel>					constel_chart_;
	std::unique_ptr<utility_aqua::DelayedCaller>	settings_delayed_;

	bool is_running_ = false;
};
