#include "SpgCore.h"
#include <iostream>
#pragma once
using namespace spg_core;

spg_core::SpgCore::SpgCore():   
    renderer_(spg_), scaler_(spg_)
{
}

void spg_core::SpgCore::SetTimeBounds(const Limits<double>& power_bounds)
{
}


void spg_core::SpgCore::SetFreqBounds(const Limits<double>& freq_bounds)
{
}

bool spg_core::SpgCore::AccumulateNewData(const std::vector<float>& passed_data, const double pos_ratio)
{
    const int column_index = std::round (pos_ratio * spg_.base_data.size.horizontal); //Определяем индекс колонки
    if(spg_.base_data.data.empty()) Emplace();

    SetDataToColumn(passed_data, column_index);
    return true;
}

QPixmap & spg_core::SpgCore::GetRelevantPixmap(const ChartScaleInfo & scale_info)
{
    // TODO: insert return statement here
    return renderer_.GetRelevantPixmap(scale_info);
}

spg_core::spg_data const & spg_core::SpgCore::GetSpectrogramInfo() const
{
    return spg_;
}

void spg_core::SpgCore::SetDataToColumn(const std::vector<float>& passed_data, size_t column_idx)
{
    if(spg_.base_data.relevant_vec[column_idx]) return; //if allready is relevant - do nothing
    const auto height = spg_.base_data.size.vertical;
    if(passed_data.size() != height) return;
    tbb::spin_mutex::scoped_lock guard_lock(spg_.rw_mutex_);
    for(int height_iter=0; height_iter < height; height_iter++)
    {
        spg_.base_data[column_idx][height_iter] = passed_data[height_iter];
    }
    spg_.base_data.need_redraw = true;
    spg_.base_data.relevant_vec[column_idx] = true;
}

spg_core::SpgCore::~SpgCore()
{
}

bool spg_core::SpgCore::Emplace()
{
    auto &basic = spg_.base_data;
    if(basic.val_bounds.horizontal.delta() <= 0) basic.val_bounds.horizontal = {0, 1000};              // Set x-axis Limits
    if(basic.val_bounds.vertical.delta()   <= 0) basic.val_bounds.vertical   = {0.0, 1.0};             // Set y-axis Limits
    basic.size.vertical         = 256;                // Set data matrix height
    basic.size.horizontal       = 1000 ;              // Set data matrix width

    // Check for valid dimensions before resizing
    if (basic.size.vertical == 0 || basic.size.horizontal == 0) {
        std::cerr << "Error: Initial DPX data dimensions cannot be zero." << std::endl;
        return false;
    }

    const size_t data_size = static_cast<size_t>(basic.size.vertical) * basic.size.horizontal;  // Total number of data elements
    basic.data.clear();                      // Clear any existing data
    basic.data.resize(data_size, 0);         // Allocate and zero-initialize data
    basic.relevant_vec.assign(basic.size.horizontal, 0); // Use assign for clear and resize
    basic.need_redraw = true;                // Set redraw flag
    return true;                                 // Initialization successful
}

void spg_core::SpgCore::Initialise(const freq_params & freq_params, const size_t samples_count)
{
}
