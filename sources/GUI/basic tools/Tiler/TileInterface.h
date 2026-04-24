#pragma once
#include "GUI/gui_defs.h"
using namespace aqua_gui;
struct TilerUnit {
	HorVerLim<double>	val_bounds;
	HV_Info<size_t>		data_size;

	std::vector<int>	data; //наша карта плотностей

	//-Надо разделить!

	//(Актуально для DPX)
	std::vector<size_t>	column_weight; //Вес каждой колонки 

	//(Актуально для SPG) - SPG было бы хорошо перевести транспарентный вид, но пока что так....
	std::vector<bool>	relevant_vec;	// Актуальна ли колонка
	std::vector<double> pos_vec;		// Вектор реальных позиций колонок
};

class TileInterface {
public:
	void				SetValBounds			( HorVerLim<double> bounds );
	void				Reset					();
	HorVerLim<double>	GetValBounds			();
	void				SetData					( const draw_data& data );
	const TilerUnit&	GetDataUnit				();
	void				UpdateDataFromDataUnit	( const TilerUnit& passed_data ); // + Update From current?...

protected:
	TilerUnit unit_;
};
