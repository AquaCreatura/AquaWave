#pragma once
#include "GUI/gui_defs.h"
using namespace aqua_gui;


class TileInterface {
public:
	using uptr = std::unique_ptr<TileInterface>;
	void					SetValBounds			( HorVerLim<double> bounds );
	HorVerLim<double>		GetValBounds			();
	void					SetImageSize			(const HV_Info<size_t> size);
	const HV_Info<size_t>	GetImageSize			() const;
	std::unique_ptr<TileInterface>	RecreateWithBounds(const HorVerLim<double>& bounds) const;

	virtual void			SetData					( const draw_data& data							   ) = 0;
	virtual void			UpdateFromTile			(const TileInterface* passed_data) = 0; 	// + Update From current?...
	virtual void			UpdateQimage			( dynamic_qimage& dyn_qimage, const Limits<double> &power_bounds) = 0;
	virtual void			Reset					() = 0;
protected:
public:
	std::atomic_bool	is_data_updated_;
	HorVerLim<double>	val_bounds_; 
	double last_average_density_{ 0.0 };
	double max_density_{ 0.0 };

	std::vector<bool>	relevant_vec_;	// Актуальна ли колонка
protected:
	HV_Info<size_t>		data_size_;
	bool				is_rotated_{ false };
	bool				is_spg_{false};

};
