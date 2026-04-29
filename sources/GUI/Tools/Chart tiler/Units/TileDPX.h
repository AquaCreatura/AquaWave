#pragma once
#include "TileInterface.h"
class TileDPX : public TileInterface
{
	virtual void			SetData			(const draw_data& data);
	virtual void			UpdateFromTile	(const TileInterface::uptr& passed_data); 	// + Update From current?...
	virtual void			UpdateQimage	(dynamic_qimage& dyn_qimage);
private:
	void DrawOnlyPoints	 (const std::vector<float> &passed_data, const Limits<double>& x_bounds);
	void DrawInterpolated(const std::vector<float> &passed_data, const Limits<double>& x_bounds);

	//(Актуально для DPX)
	std::vector<size_t>	column_weight; //Вес каждой колонки 
};