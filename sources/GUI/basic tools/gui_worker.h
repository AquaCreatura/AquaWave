#pragma once
#include <vector>
#include <algorithm>
#include "GUI/gui_defs.h"
namespace aqua_gui
{

// Adjusts chart scale based on mouse wheel delta and scale point
bool ZoomFromWheelDelta(ChartScaleInfo& scale_info, const int wheel_delta, const QPoint scale_point);
void AdaptPowerBounds (ChartScaleInfo& scale_info, const Limits<double>& new_bounds);

} // namespace aqua_gui