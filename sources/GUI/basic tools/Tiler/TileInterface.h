#pragma once
#include "GUI/gui_defs.h"
//Отказаться от Unit -> Объединить с Tile!!
using namespace aqua_gui;
struct TilerUnit {
	HorVerLim<double>	val_bounds;
	HV_Info<size_t>		data_size;

	std::vector<int>	data; //наша карта плотностей
	bool				is_data_updated;
	tbb::spin_mutex		mutex;
};

class TileInterface {
public:
	void				SetValBounds			( HorVerLim<double> bounds );
	HorVerLim<double>	GetValBounds			();
	const TilerUnit&	GetDataUnit();
	virtual void		Reset();

	virtual void		SetData					( const draw_data& data		   ) = 0;
	virtual void		UpdateDataFromDataUnit	( const TilerUnit& passed_data ) = 0; 	// + Update From current?...
	virtual void		UpdateQimage			( dynamic_qimage& dyn_qimage   ) = 0;

	TilerUnit unit_;
};
