#pragma once
#include "TileInterface.h"
class TileDPX : public TileInterface
{



public:
	//(Актуально для SPG) - SPG было бы хорошо перевести транспарентный вид, но пока что так....
	std::vector<bool>	relevant_vec;	// Актуальна ли колонка
	std::vector<double> pos_vec;		// Вектор реальных позиций колонок
};