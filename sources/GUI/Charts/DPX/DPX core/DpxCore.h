#pragma once
#include <vector>
#include <ipps.h>
#include "DPX_defs.h"
#include "DpxRenderer.h"
#include "DpxScaler.h"
using namespace fluctus;
namespace dpx_core
{



/**
 * @brief Core component for DPX data processing. Handles initialization and data ingestion.
 */
class DpxCore
{
public:
    DpxCore();
    /**
     * @brief Initializes the internal data structures and states.
     * @return true if initialization succeeded.
     */
    bool Init();

    /**
     * @brief Feeds new data values into the core, adjusts Limits and processes using one of two algorithms.
     * @param passed_data Input array of values.
     * @param x_bounds Range of X-axis to associate with the data.
     * @return true if data was successfully ingested.
     */
    bool PassNewData(const std::vector<float>& passed_data, const Limits<double>& x_bounds);

    /**
     * @brief Sets the minimum and maximum X Limits for the data.
     * @param x_bounds H pair containing the new minimum and maximum X values.
     * @param hard_reset If true, performs a hard reset of the data; otherwise, adjusts accordingly.
     * @return true if the operation was successful.
     */
    bool SetMinMax_X(const Limits<double>& x_bounds, const bool hard_reset = false);

    /**
     * @brief Retrieves the current Y Limits (power Limits) of the data.
     * @return H pair containing the minimum and maximum Y values.
     */
    Limits<double> GetPowerBounds() const;
    //Used to update power bounds of our image
    void           SetPowerBounds   (const Limits<double>& x_bounds);

    //Is Used to get pixmap from our dpx
    QPixmap&       GetRelevantPixmap(const ChartScaleInfo& scale_info);
private:
    /**
     * @brief Processes the input data directly, assuming passed_data size is greater than or equal to dpx_data_.size.horizontal.
     * @param passed_data Input values to process.
     * @param x_bounds Range of X-axis corresponding to the data.
     * @return true if processing succeeded.
     */
    bool RoughPassedLoop(const std::vector<Ipp32f> &passed_data, const Limits<double>& x_bounds);

    /**
     * @brief Processes the input data using linear interpolation between neighboring values.
     * Used when dpx_data_.size.horizontal > passed_data.size().
     * @param passed_data Input values to interpolate.
     * @param x_bounds Range of X-axis corresponding to the data.
     * @return true if interpolation succeeded.
     */
    bool SlopePassedLoop(const std::vector<Ipp32f> &passed_data, const Limits<double>& x_bounds);

protected:
    DpxDataScaler dpx_scaler_; // Responsible for Limits and dimension control
    dpx_data      dpx_data_;   // Main data structure
    DpxRenderer   dpx_renderer_;
};

} // namespace dpx_core