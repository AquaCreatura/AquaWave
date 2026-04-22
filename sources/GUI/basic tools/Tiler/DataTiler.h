#pragma once
#include "GUI/gui_defs.h"
using namespace aqua_gui;
struct TilerUnit {
	HorVerLim<double>	val_bounds;
	HV_Info<int>		px_size;

};
class DataTiler {
public:
	void SetValBounds(HV_Info<double> bounds);
	void SetData(const draw_data& data);
protected:
	TilerUnit data_;
};
