#include "DpxCore.h"
#include <algorithm>
#include <cmath>
#include <limits> // Required for std::numeric_limits
#include <iostream> // For error/warning output

// Assuming aqua_gui is a global namespace or included differently
// If not, 'using namespace aqua_gui;' might cause issues.
// For now, I'll keep it as it was in your original code.
using namespace aqua_gui;

namespace dpx_core
{


//============================================================================== DpxCore ===========================================================

DpxCore::DpxCore() : dpx_scaler_(dpx_data_), dpx_renderer_(dpx_data_) 
{
    Emplace();
    // Default constructor; no init required as Init() is called separately
}

bool DpxCore::Emplace() {
	// Обновляем данные
	tbb::spin_mutex::scoped_lock scoped_locker(dpx_data_.redraw_mutex);

    if(dpx_data_.val_bounds.horizontal.delta() <= 0) dpx_data_.val_bounds.horizontal = {0, 1000};              // Set x-axis Limits
    if(dpx_data_.val_bounds.vertical.delta()   <= 0) dpx_data_.val_bounds.vertical   = {0.0, 1.0};             // Set y-axis Limits
    dpx_data_.size.vertical         = 1'024 / 5;                // Set data matrix height
    dpx_data_.size.horizontal       = 1'024 * 4 ;              // Set data matrix width

    // Check for valid dimensions before resizing
    if (dpx_data_.size.vertical == 0 || dpx_data_.size.horizontal == 0) {
        std::cerr << "Error: Initial DPX data dimensions cannot be zero." << std::endl;
        return false;
    }

    const size_t data_size = static_cast<size_t>(dpx_data_.size.vertical) * dpx_data_.size.horizontal;  // Total number of data elements
    dpx_data_.data.clear();                      // Clear any existing data
    dpx_data_.data.resize(data_size, 0);         // Allocate and zero-initialize data
    dpx_data_.column_weight.assign(dpx_data_.size.horizontal, 0); // Use assign for clear and resize
    dpx_data_.need_redraw = true;                // Set redraw flag
    return true;                                 // Initialization successful
}

bool DpxCore::AccumulateNewData(const std::vector<float>& passed_data, const Limits<double>& x_bounds) {
    tbb::spin_mutex::scoped_lock scoped_locker(dpx_data_.redraw_mutex);
    if (passed_data.empty()) {
        std::cerr << "Warning: Passed data is empty in PassNewData." << std::endl;
        return false;       // Return if input is empty
    }
    dpx_data_.need_redraw = true;
    // Choose loop strategy based on data size vs. horizontal resolution
    // Ensure horizontal is not zero to avoid division by zero in loop implementations
    if (dpx_data_.size.horizontal == 0) {
        std::cerr << "Error: dpx_data_.size.horizontal is zero. Cannot process data." << std::endl;
        return false;
    }
    const double samples_per_pixel =  double(passed_data.size()) / dpx_data_.size.horizontal;
    if (samples_per_pixel > 1 ) {
        return RoughPassedLoop(passed_data, x_bounds); // Use original x_bounds from input
    } else {
        return SlopePassedLoop(passed_data, x_bounds); // Use original x_bounds from input
    }
}

bool DpxCore::SetMinMax_X(const Limits<double>& new_x_bounds)
{
    // Only call UpdateBounds_x if the bounds actually change
    if (new_x_bounds!= dpx_data_.val_bounds.horizontal) {
        return dpx_scaler_.UpdateBounds_x(new_x_bounds);
    }
    return true; // No change in bounds, considered successful
}

Limits<double> DpxCore::GetPowerBounds() const
{
    return dpx_data_.val_bounds.vertical;
}

void DpxCore::SetPowerBounds(const Limits<double>& x_bounds)
{
    dpx_scaler_.UpdateBounds_y(x_bounds);
}


QPixmap & DpxCore::GetRelevantPixmap(const ChartScaleInfo & scale_info)
{
    auto &min_max = scale_info.val_info_.min_max_bounds_;
    SetPowerBounds(min_max.vertical); //Обновляем границы мощности
    SetMinMax_X   (min_max.horizontal); //Обновляем диапазон значений
    return dpx_renderer_.GetRelevantPixmap(scale_info);
}

void DpxCore::SetFftOrder(int n_fft_order)
{
	tbb::spin_mutex::scoped_lock scoped_locker(dpx_data_.redraw_mutex);
	dpx_data_.n_fft_ = 1 << n_fft_order;
}


/**
 * @brief  Distributes a sequence of y-values into a 2D histogram-like grid (dpx_data_),
 * incrementing the appropriate bin and its column weight for each point.
 *
 * @param  passed_data  A vector of y-values to be binned.
 * @param  x_bounds     A pair specifying the inclusive lower and upper Limits along the x-axis.
 * @return true         Always returns true to indicate the loop completed without errors.
 */
bool DpxCore::RoughPassedLoop(
    const std::vector<float>& passed_data,
    const Limits<double>& x_bounds)
{

    // Unpack x-axis limits of the incoming data.
    const double input_x_min = x_bounds.low;
    const double input_x_max = x_bounds.high;

    // Compute dimensions of the DPX grid.
    const size_t grid_width = static_cast<size_t>(dpx_data_.size.horizontal);  // Number of bins along x-axis
    const size_t grid_height = static_cast<size_t>(dpx_data_.size.vertical);   // Number of bins along y-axis

    // Total span along x for the current DPX data display range.
    auto &dpx_horizontal_bounds = dpx_data_.val_bounds.horizontal;
    const double dpx_x_range = dpx_horizontal_bounds.delta();
    const double dpx_x_start = dpx_horizontal_bounds.low;
    const double dpx_x_bin_width = dpx_x_range / static_cast<double>(grid_width);

    // Starting positions (lower limits) for y-axis mapping.
    auto &dpx_vertical_bounds = dpx_data_.val_bounds.vertical;
    const double dpx_y_start = dpx_vertical_bounds.low;
    const double dpx_y_step = dpx_vertical_bounds.delta() / dpx_data_.size.vertical;
    const double dpx_y_step_inverse = 1.0 / dpx_y_step;

    // Calculate how much real-world x advances per incoming input sample.
    const double input_x_range = input_x_max - input_x_min;
    // Use size-1 for step size for range, assuming 'passed_data.size()' is at least 1.
    const double input_x_step = input_x_range / static_cast<double>(passed_data.size() - 1);

    // Iterate over each input sample to map it to the DPX grid.
    for (size_t i = 0; i < passed_data.size(); ++i)
    {
        // Determine the real-world x-position for the current sample based on incoming data bounds.
        const double current_x_position = input_x_min + static_cast<double>(i) * input_x_step;

        // Map current_x_position into a bin index using DPX data's x-bounds.
        size_t x_bin_index = static_cast<size_t>((current_x_position - dpx_x_start) / dpx_x_bin_width);

        // Clamp x_bin_index to valid range [0, grid_width - 1].
        if (x_bin_index >= grid_width) {
            // If current_x_position is exactly dpx_horizontal_bounds.high, map it to the last valid column.
            if (x_bin_index == grid_width && current_x_position == dpx_horizontal_bounds.high) {
                x_bin_index = grid_width - 1;
            } else {
                continue; // Skip if outside grid.
            }
        }

        // Read the y-value and map it to a bin index.
        const double current_y_value = static_cast<double>(passed_data[i]);
        size_t y_bin_index = static_cast<size_t>((current_y_value - dpx_y_start) * dpx_y_step_inverse);

        // Clamp y_bin_index to valid range [0, grid_height - 1].
        if (y_bin_index >= grid_height) {
            // If current_y_value is exactly dpx_vertical_bounds.high, map it to the last valid row.
            if (y_bin_index == grid_height && current_y_value == dpx_vertical_bounds.high) {
                y_bin_index = grid_height - 1;
            } else {
                continue; // Skip if outside grid.
            }
        }

        // Increment the count in the corresponding bin and update column weight.
        dpx_data_[y_bin_index][x_bin_index] += 1;
        dpx_data_.column_weight[x_bin_index] += 1;
    }

    dpx_data_.need_redraw = true; // Set redraw flag to indicate data change.
return true;
}

bool DpxCore::SlopePassedLoop(const std::vector<float>& passed_data,
                              const Limits<double>& passed_bounds)
{
    // Проверка входных данных: необходимо как минимум 2 точки для линейной интерполяции
    const size_t input_point_count = passed_data.size();
    if (input_point_count < 2) {
        return false;
    }

    // Расчет базовых параметров входных данных и сетки DPX
    const double input_x_range = static_cast<double>(passed_bounds.high - passed_bounds.low);
    const double input_x_step = input_x_range / (input_point_count - 1);  // шаг по X для входных данных

    const auto& x_bounds_dpx = dpx_data_.val_bounds.horizontal;
    const auto& y_bounds_dpx = dpx_data_.val_bounds.vertical;

    const size_t grid_width  = static_cast<size_t>(dpx_data_.size.horizontal);  // количество колонок
    const size_t grid_height = static_cast<size_t>(dpx_data_.size.vertical);    // количество строк

    const double dpx_x_start = x_bounds_dpx.low;
    const double dpx_x_bin_width = x_bounds_dpx.delta() / static_cast<double>(grid_width);

    const double dpx_y_start = y_bounds_dpx.low;
    const double dpx_y_step = y_bounds_dpx.delta() / static_cast<double>(grid_height);
    const double inv_dpx_y_step = 1.0 / dpx_y_step;

    // Основной проход по сегментам (между каждой парой точек)
    for (size_t i = 0; i < input_point_count - 1; ++i) {
        const SlopeInterpolator interpolator(passed_data[i], passed_data[i + 1]);

        // X-границы текущего сегмента
        const double seg_x0 = passed_bounds.low + i * input_x_step;
        const double seg_x1 = passed_bounds.low + (i + 1) * input_x_step;

        // Грубое определение колонок сетки, которые пересекает сегмент
        int64_t grid_x_start_idx = static_cast<int64_t>((seg_x0 - dpx_x_start) / dpx_x_bin_width);
        int64_t grid_x_end_idx   = static_cast<int64_t>((seg_x1 - dpx_x_start) / dpx_x_bin_width);

        if (seg_x1 > seg_x0) {
            grid_x_end_idx = static_cast<int64_t>(
                std::ceil((seg_x1 - dpx_x_start) / dpx_x_bin_width)
            );
        }

        // Ограничение диапазона в пределах сетки DPX
        size_t bin_x_start = static_cast<size_t>(std::max(int64_t(0), grid_x_start_idx));
        size_t bin_x_end   = static_cast<size_t>(std::min(static_cast<int64_t>(grid_width), grid_x_end_idx + 1));

        if (grid_width > 0 && bin_x_start >= grid_width) bin_x_start = grid_width - 1;
        if (grid_width > 0 && bin_x_end > grid_width)     bin_x_end = grid_width;

        // Заполнение соответствующих ячеек сетки
        for (size_t x_bin = bin_x_start; x_bin < bin_x_end; ++x_bin) {
            const double x_center = dpx_x_start + static_cast<double>(x_bin) * dpx_x_bin_width;

            // Нормализованная координата вдоль сегмента [0, 1]
            double t = (x_center - seg_x0) / input_x_step;
            if (t < 0.0) t = 0.0;
            if (t > 1.0) t = 1.0;

            const double y_val = interpolator.Interpolate(t);

            // Индекс строки сетки по Y
            size_t y_bin = static_cast<size_t>((y_val - dpx_y_start) * inv_dpx_y_step);
            if (y_bin >= grid_height) {
                if (grid_height > 0) y_bin = grid_height - 1;
                else continue;
            }

            // Инкремент значения в ячейке и веса колонки
            if (x_bin < grid_width && y_bin < grid_height) {
                dpx_data_[y_bin][x_bin] += 1;
                dpx_data_.column_weight[x_bin] += 1;
            } else {
                std::cerr << "Error: Index out of bounds during SlopePassedLoop binning. x=" << x_bin << ", y=" << y_bin << std::endl;
            }
        }
    }

    // Устанавливаем флаг необходимости перерисовки
    dpx_data_.need_redraw = true;
    return true;

}

} // namespace dpx_core