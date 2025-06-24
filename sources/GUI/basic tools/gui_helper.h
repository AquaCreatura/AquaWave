#pragma once
#include <vector>
#include <algorithm>
#include "GUI/gui_defs.h"
#include "gui_worker.h"
namespace aqua_gui
{

// Linear interpolator between two values
class SlopeInterpolator
{
public:
    // Initialize with first and next values, compute slope
    SlopeInterpolator(const double first_val, const double next_val)
        : first_val_(first_val), slope_koeff_(next_val - first_val) {}
    
    // Interpolate value at ratio [0, 1) between first and second value
    constexpr double Interpolate(double ratio) const noexcept
    {
        return first_val_ + (ratio * slope_koeff_);
    }

private:
    const double first_val_ = 0;    // First value
    const double slope_koeff_ = 0;  // Slope between first and next value
};

// Returns normalized position [0, 1] of element's center for given iteration
const double GetAsimDrawPlace(const int iteration_counter);

// Returns vector mapping iteration indices to unique pixel positions [0, width-1]
std::vector<int> GetAssimLocationsVec(const int width);

} // namespace aqua_gui