#pragma once
#include <vector>
#include <algorithm>
#include "GUI/gui_defs.h"
namespace aqua_gui
{

// Adjusts chart scale based on mouse wheel delta and scale point
bool ZoomFromWheelDelta(ChartScaleInfo& scale_info, const int wheel_delta, const QPoint scale_point);
void AdaptVertPowerBounds (ChartScaleInfo& scale_info);
bool PanFromMouse(ChartScaleInfo& scale_info, const QPoint start_mouse_point, 
									const HV_Info<double, double>& /*start_hor_ver*/, const QPoint end_mouse_point);

} // namespace aqua_gui