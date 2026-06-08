#pragma once
#include "TileInterface.h"
#include "Utilities/calc_tools.h"
class TileDPX : public TileInterface
{
public:
	TileDPX();
	virtual void			SetData				(const draw_data& data) override;
	virtual void			UpdateFromTile		(const TileInterface* passed_data) override; 	// + Update From current?...
	virtual void			UpdateQimage		(dynamic_qimage& dyn_qimage, const Limits<double> &power_bounds) override;
	void					Reset				() override;	

private:
	void					DrawOnlyPoints		(const std::vector<float> &passed_data, const Limits<double>& x_bounds);
	void					DrawInterpolated	(const std::vector<float> &passed_data, const Limits<double>& x_bounds);
	argb_t					GetNormColor		(const double density) const;
	void					PrepareForNewData	();
	//(Актуально для DPX)
	utility_aqua::DataSpeedEstimator data_speedometer_;
	std::vector<size_t>		column_weight_vec_; //Вес каждой колонки 
	std::vector<int64_t>	data_;			//наша карта плотностей
	size_t					max_life_time_ms_;			//Максимальный вес колонки, относительно которого храним данные
	size_t					max_trans_life_time_ms_;	//Максимальный вес, относительно которого минусуем при переходном процессе
	int64_t					trans_decrease_counter_;		//Уменьшается на значение, равное минимальной дельте колонок
};