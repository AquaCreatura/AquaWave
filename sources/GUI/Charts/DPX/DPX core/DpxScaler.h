#pragma once
#include "DPX_defs.h"
#include "qpixmap.h"
#include <qimage.h>
#include "GUI/Drawers/QimageZoomer.h"
#include "GUI/basic tools/gui_helper.h"
#include "GUI/basic tools/gui_conversions.h"
namespace dpx_core
{
/**
 * @brief Utility for rescaling and managing Limits and dimensions of dpx_data.
 */
class DpxDataScaler
{
public:
    /**
     * @brief Constructor initializes scaler with reference to target dpx_data object.
     * @param init_val Reference to a dpx_data object to operate on.
     */
    DpxDataScaler(dpx_data &init_val);

    /**
     * @brief Rescales X-axis Limits. If hard_reset is true, clears data and resets dimensions.
     * Otherwise, performs interpolation using SlopeInterpolator.
     * @param new_bounds Target X-axis Limits.
     * @param hard_reset Whether to forcibly reset and clear data.
     * @return true if update was successful.
     */
    bool UpdateBounds_x(const Limits<double>& new_bounds);

    /**
     * @brief Updates Y-axis Limits if input values exceed current Limits.
     * Adds a 10% margin on both sides. Interpolates or resizes data if necessary.
     * @param val_bounds New min/max value range to consider.
     * @return true if update or adjustment was successful.
     */
    bool UpdateBounds_y(const Limits<double>& val_bounds);

protected:
    dpx_data &data_;  // Reference to the underlying data structure
};
}
