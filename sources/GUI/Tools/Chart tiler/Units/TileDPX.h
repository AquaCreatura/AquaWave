#pragma once
#include "TileInterface.h"
#include "Utilities/calc_tools.h"

class TileDPX : public TileInterface
{
public:
	TileDPX();

	virtual void SetData(const draw_data& data) override;
	virtual void UpdateFromTile(const TileInterface* passed_data) override;
	virtual void UpdateQimage(dynamic_qimage& dyn_qimage, const Limits<double>& power_bounds) override;
	void Reset() override;
	virtual	void SetDpxParams(const double fps, const double time_hold_sec);
private:
	// Отрисовка точек и интерполяция
	void DrawOnlyPoints(const std::vector<float>& passed_data, const Limits<double>& x_bounds);
	void DrawInterpolated(const std::vector<float>& passed_data, const Limits<double>& x_bounds);

	// Преобразование плотности в цвет
	argb_t GetNormColor(double density) const;

	// Применение глобального затухания ко всем данным
	void ApplyDecay();

	// Пересчёт максимума и среднего для коррекции цвета
	void UpdateDensityPivot();

	// Основные буферы
	std::vector<size_t>   column_weight_vec_;   // вес каждой колонки (сумма значений по вертикали)
	std::vector<int64_t>  data_;                // карта плотностей (height * width)

												// Параметры времени жизни (используются для расчёта скорости затухания)

											   // Счётчик, активирующий ускоренное затухание (например, при зуме)
	int64_t trans_decrease_counter_ = 0;

	// Параметры затухания
	double  decay_factor_ = 1.0;       // текущий множитель затухания (применяется за кадр)
	double  base_decay_rate_ = 1.0;   // базовая скорость затухания (1.0 – без затухания)
	double  trans_decay_rate_ = 1.0;   // переходная скорость затухания (1.0 – без затухания)

									   // Измеритель скорости поступления данных (опционально, для статистики)
	utility_aqua::DataSpeedEstimator data_speedometer_;
};