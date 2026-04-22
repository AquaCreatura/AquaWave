#pragma once
#include "DataTiler.h"
using namespace aqua_gui;
class ChartTiler {

public:
	//Init bounds of base image
	void InitBaseBounds	(const HorVerLim<double> bounds);
	void SetViewBounds	(const HorVerLim<double> bounds);
	void SetData		(const draw_data& data);



};