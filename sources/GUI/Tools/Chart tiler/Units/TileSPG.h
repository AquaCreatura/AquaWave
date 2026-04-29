#pragma once
#include "TileInterface.h"
class TileSPG : public TileInterface
{
	virtual void			SetData(const draw_data& data);
	virtual void			UpdateFromTile(const TileInterface::uptr& passed_data); 	// + Update From current?...
	virtual void			UpdateQimage(dynamic_qimage& dyn_qimage);
private:
	//(Актуально для SPG) - SPG было бы хорошо перевести транспарентный вид, но пока что так....
	std::vector<double> pos_vec;		// Вектор реальных позиций колонок
};