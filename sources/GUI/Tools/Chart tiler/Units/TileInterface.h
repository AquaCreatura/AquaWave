#pragma once
#include "GUI/gui_defs.h"
using namespace aqua_gui;


class TileInterface {
public:
	void					SetValBounds			( HorVerLim<double> bounds );
	HorVerLim<double>		GetValBounds			();

	void					SetImageSize			(const HV_Info<size_t>& size);
	const HV_Info<size_t>&	GetImageSize			();

	virtual void			Reset();

	virtual void			SetData					( const draw_data& data		   ) = 0;
	virtual void			UpdateFromTile			( const std::unique_ptr<TileInterface>& passed_data) = 0; 	// + Update From current?...
	virtual void			UpdateQimage			( dynamic_qimage& dyn_qimage   ) = 0;

public:
	std::atomic_bool	is_data_updated_;
	tbb::spin_mutex		mutex_;
private:
	HorVerLim<double>	val_bounds_;
	HV_Info<size_t>		image_size_;
	std::vector<int>	data_; //наша карта плотностей


};
