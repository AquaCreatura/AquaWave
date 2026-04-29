#pragma once
#include "TileInterface.h"
class TileDPX : public TileInterface
{



private:
	//(Актуально для SPG) - SPG было бы хорошо перевести транспарентный вид, но пока что так....
	std::vector<double> pos_vec;		// Вектор реальных позиций колонок
};