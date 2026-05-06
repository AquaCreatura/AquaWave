#pragma once
#include "TileInterface.h"
class TileDPX : public TileInterface
{
public:
	virtual void			SetData			(const draw_data& data) override;
	virtual void			UpdateFromTile	(const TileInterface::uptr& passed_data) override; 	// + Update From current?...
	virtual void			UpdateQimage	(dynamic_qimage& dyn_qimage, const Limits<double> &power_bounds) override;
	void					Reset			() override;	
private:
	void DrawOnlyPoints	 (const std::vector<float> &passed_data, const Limits<double>& x_bounds);
	void DrawInterpolated(const std::vector<float> &passed_data, const Limits<double>& x_bounds);
	argb_t					GetNormColor(const double density) const;
	//(Актуально для DPX)
	std::vector<size_t>	column_weight; //Вес каждой колонки 
	std::vector<int64_t>	data_;			//наша карта плотностей
};