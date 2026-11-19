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
#include <vector>

using namespace utility_aqua;

class AquaDemodWindow : public QDialog
{
	Q_OBJECT
public:
	// is_light_mode: окно всегда в компактном виде (встраивается, например, в
	// ScopeAnalyser). Иначе компактный режим включается сам при уменьшении окна
	// и выключается при увеличении.
	AquaDemodWindow(bool is_light_mode = false);
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
	QString	GetEqAlgorithm() const;
	double	GetCarrierHz() const;
	double	GetSymbolRateHz() const;
	double	GetPllSpeed() const;
	double	GetTllSpeed() const;
	double	GetEqSpeed() const;

	// Отрисовка созвездия и обновление оценочных значений.
	void PushConstelData(std::vector<Ipp32fc>& constel_data);
	void ClearConstelData();
	// Опорные точки созвездия для графика: масштаб и маркеры поверх данных.
	void SetConstelPivots(const std::vector<Ipp32fc>& pivots);
	void ClearConstelPivots();
	void UpdateEstimates(double snr_db, double carrier_offset_hz, double symbol_rate_hz);
	// Переключение между полной и компактной раскладками: для встроенного окна
	// применяет светлую раскладку, для отдельного — скрывает/показывает pll_frame.
	void ApplyCompactMode(bool compact);
signals:
	// Изменились «тяжёлые» параметры демодуляции — требуется пересборка тракта.
	void DemodSettingsChanged();
	// Скорости в единицах реальных параметров петель (см. SetSpeedRanges).
	void PllSpeedChanged(double speed);
	void TllSpeedChanged(double speed);
	void EqSpeedChanged(double speed);
	// Сменился алгоритм эквалайзера (MMA/CMA/LMS), применяется на лету.
	void EqAlgorithmChanged(const QString& algorithm);
	void RunToggled(bool running);
	// Запрос на сброс ФАПЧ, усиления, эквалайзера и т.п.
	void ResetRequested();
	// Сменился вид модуляции (для отображения текущего значения снаружи).
	void ModulationChanged(const QString& modulation);

protected:
	// Создаёт график созвездия и кладёт его в отведённый фрейм.
	void SetupConstellationChart();
	// Создаёт кнопки и запоминает их обычные размеры.
	void SetupButtons();
	// Список модуляций и смена текущего значения.
	void SetupModulations();
	// Список алгоритмов эквалайзера и смена текущего значения.
	void SetupEqAlgorithms();
	// Группы петель (label, слайдер, спинабокс) для PLL/TLL/эквалайзера.
	void SetupSliders();
	// Границы SNR и связь ручного ввода частот со сменой параметров.
	void SetupRanges();
	// Размер и картинки кнопок в заданном виде (полный/компактный).
	void ApplyButtonSizes(bool compact);
	// Светлая раскладка встроенного окна: созвездие, SNR, модуляция,
	// Start/Reset; несущая и символьная скорость из анализатора.
	void SetupLightLayout();
	void OnStartStopClicked();
	void OnResetClicked();

private:
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

	struct SliderInfo
	{
		aqua::Limits<double> Limits;
		int Steps = 1;
	};

	int SliderValue(QSlider* slider, double speed) const;
	double SliderSpeed(QSlider* slider) const;
	AquaDemodWindow::SliderInfo SliderParams(QSlider* slider) const;
	QString FormatRangeEnd(double value);
	void UpdateRunUI();

	Ui::AquaDemodWindow								ui_;
	QPointer<constel::ChartConstel>					constel_chart_;
	std::unique_ptr<utility_aqua::DelayedCaller>	settings_delayed_;

	// Светлая (компактная) раскладка встроенного окна.
	QWidget*			light_frame_ = nullptr;
	QWidget*			light_params_frame_ = nullptr;
	QGridLayout*		light_params_grid_ = nullptr;
	QGridLayout*		light_values_grid_ = nullptr;
	QHBoxLayout*		light_buttons_ = nullptr;
	QHBoxLayout*		light_body_ = nullptr;
	QSize				full_button_min_;
	QSize				full_button_max_;

	bool is_light_mode_ = false;   // окно всегда компактное (встроено)
	bool compact_active_ = false;
	bool is_running_ = false;
};
