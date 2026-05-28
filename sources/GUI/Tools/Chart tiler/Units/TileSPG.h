#pragma once
#include "TileInterface.h"
class TileSPG : public TileInterface
{
public:
	TileSPG();
	void			SetData(const draw_data& data) override;
	void			UpdateFromTile(const TileInterface* passed_data) override; 	// + Update From current?...
	void			UpdateQimage(dynamic_qimage& dyn_qimage, const Limits<double> &power_bounds) override;
protected:
	void					SetDataToRow(const float * passed_data, int data_size, const Limits<size_t> dst_bounds, double time_pos, const bool is_relevant);
	argb_t					GetNormColor(const double density) const;
	void					Reset() override;

private:
	std::vector<double> pos_vec_; // Вектор реальных позиций колонок
	std::vector<float>	data_;			//наша карта плотностей
	

};

