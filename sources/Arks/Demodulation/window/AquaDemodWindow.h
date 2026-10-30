#pragma once
#include "Utilities/utility_aqua.h"
#include "Utilities/qt_utility.h"
#include "GUI/Charts/Constel/ConstelChart.h"

#include <qdialog.h>
#include <qpointer.h>
#include "ui_AquaDemodWindow.h"

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
	void PllSpeedChanged(double speed);
	void TllSpeedChanged(double speed);
	void EqSpeedChanged(double speed);

protected:
	void SetupModulations();
	void SetupSliders();
	void SetupRanges();

protected:
	Ui::AquaDemodWindow								ui_;
	QPointer<constel::ChartConstel>					constel_chart_;
	std::unique_ptr<utility_aqua::DelayedCaller>	settings_delayed_;
};
