#pragma once
#include "TileInterface.h"
#include <future>
using namespace aqua_gui;
//Works synchoniously
class ChartTiler { 

public:
	//Init bounds of base image
	void InitBaseBounds	(const HorVerLim<double> passed_bounds);
	void SetViewBounds	(const HorVerLim<double> view_bounds);
	void SetData		(const draw_data& data);
protected:
	TileInterface cur_tile_	;
	TileInterface buff_tile_;
	TileInterface base_tile_;

	bool				need_use_base_tile_{false};
	const double		zoom_step_sqrt_ = 1.5;

	HorVerLim<double>	base_bounds_;
	HorVerLim<double>	view_bounds_;
};