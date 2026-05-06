#pragma once
#include <vector>
#include <algorithm>
#include "GUI/gui_defs.h"
namespace aqua_gui
{

// Adjusts chart scale based on mouse wheel delta and scale point
bool ZoomFromWheelDelta(ChartScaleInfo& scale_info, const int wheel_delta, const QPoint scale_point);
void AdaptVertPowerBounds (ChartScaleInfo& scale_info);

} // namespace aqua_gui