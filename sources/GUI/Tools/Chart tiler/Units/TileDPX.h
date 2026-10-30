#pragma once
#include "TileInterface.h"

#include <qelapsedtimer.h>

class TileDPX : public TileInterface
{
public:
	TileDPX(const bool need_decay);

	virtual void SetData(const draw_data& data) override;
	virtual void UpdateFromTile(const TileInterface* passed_data) override;
	virtual void UpdateQimage(dynamic_qimage& dyn_qimage, const Limits<double>& power_bounds) override;
	void Reset() override;
private:
	void UpdateDecayInfo();
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
	std::vector<double>   column_weight_vec_;   // вес каждой колонки (сумма значений по вертикали)
	std::vector<float>	  data_;                // карта плотностей (height * width)

												// Параметры времени жизни (используются для расчёта скорости затухания)

											   // Счётчик, активирующий ускоренное затухание (например, при зуме)
	int64_t trans_decrease_counter_ = 0;

	// Параметры затухания
	double			decay_factor_ = 1.0;       // текущий множитель затухания (применяется за кадр)
	double			base_decay_rate_ = 1.0;   // базовая скорость затухания (1.0 – без затухания)
	double			trans_decay_rate_ = 1.0;   // переходная скорость затухания (1.0 – без затухания)
	QElapsedTimer	update_elapsed_timer_;
	QElapsedTimer	timer_after_start_decaying_;
	double				after_trans_counter_{ 1. };
	double				drawn_samples_per_column_{ 1. };
	
};
