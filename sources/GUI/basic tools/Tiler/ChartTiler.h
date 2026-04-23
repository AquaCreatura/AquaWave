#pragma once
#include "DataTiler.h"
#include <future>
using namespace aqua_gui;
//Works synchoniously
class ChartTiler { 

public:
	//Init bounds of base image
	void InitBaseBounds	(const HorVerLim<double> bounds);
	void SetViewBounds	(const HorVerLim<double> bounds);
	void SetData		(const draw_data& data);
protected:
	std::vector<DataTiler> tiles_;

};