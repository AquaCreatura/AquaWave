#pragma once
#include "GUI/gui_defs.h"
using namespace aqua_gui;


class TileInterface {
public:
	using uptr = std::unique_ptr<TileInterface>;
	void					SetValBounds			( HorVerLim<double> bounds );
	HorVerLim<double>		GetValBounds			();

	void					SetImageSize			(const HV_Info<size_t>& size);
	const HV_Info<size_t>	GetImageSize			();
	virtual void			Reset();

	virtual void			SetData					( const draw_data& data							   ) = 0;
	virtual void			UpdateFromTile			( const TileInterface::uptr& passed_data			) = 0; 	// + Update From current?...
	virtual void			UpdateQimage			( dynamic_qimage& dyn_qimage, const Limits<double> &power_bounds) = 0;

public:
	std::atomic_bool	is_data_updated_;
	tbb::spin_mutex		mutex_;
protected:
	HorVerLim<double>	val_bounds_;
	HV_Info<size_t>		data_size_;
	bool				is_rotated_{ false };
	std::vector<bool>	relevant_vec_;	// Актуальна ли колонка

	double last_max_density_{ 1.0 };
	double last_average_density_{ 1.0 };
};
