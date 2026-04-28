#pragma once
#include "TileInterface.h"
class TileDPX : public TileInterface
{



private:
	//(Актуально для DPX)
	std::vector<size_t>	column_weight; //Вес каждой колонки 
};