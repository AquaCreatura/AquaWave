#pragma once
#include "TileInterface.h"
class TileSPG : public TileInterface
{
	virtual void			SetData(const draw_data& data);
	virtual void			UpdateFromTile(const TileInterface::uptr& passed_data); 	// + Update From current?...
	virtual void			UpdateQimage(dynamic_qimage& dyn_qimage);
protected:
	void					SetDataToRow(const float* passed_data, int data_size, const size_t row_idx, const Limits<size_t> start_end_idx );
private:
	std::vector<double> pos_vec_; // Вектор реальных позиций колонок
};

