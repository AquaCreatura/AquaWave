#include "SpgCore.h"
#include <iostream>
#pragma once
using namespace spg_core;

spg_core::SpgCore::SpgCore():   
    renderer_(spg_), scaler_(spg_)
{
    Emplace();
}

void spg_core::SpgCore::SetTimeBounds(const Limits<double>& power_bounds)
{
	spg_.base_data.val_bounds.horizontal = power_bounds;
	Emplace();
}


void spg_core::SpgCore::SetFreqBounds(const Limits<double>& freq_bounds)
{
	spg_.base_data.val_bounds.vertical = freq_bounds;
	Emplace();
}

void spg_core::SpgCore::SetNfftOrder(int fft_order)
{
	tbb::spin_mutex::scoped_lock guard_lock(spg_.rw_mutex_);
	spg_.n_fft_ = 1 << fft_order;;

	spg_.base_data.state = kPrecising | kRequestStation;
	spg_.base_data.relevant_vec.assign(spg_.base_data.relevant_vec.size(), 0);

	spg_.realtime_data.state = kPrecising | kRequestStation;
	spg_.realtime_data.relevant_vec.assign(spg_.realtime_data.relevant_vec.size(), 0);
}

bool spg_core::SpgCore::AccumulateNewData(const std::vector<float>& passed_data, const double pos_ratio)
{
	if (passed_data.size() != spg_.n_fft_) return false;
	const auto src_bounds = spg_.base_data.val_bounds;
	auto SetRatioToMatrix = [&](spg_holder& data_holder) -> bool {
		if (data_holder.state == HolderStation::kFullData) return true;
		//Необходимо перевести в относитльный вид, относительно текущего диапазона

		auto &hor_bounds  = data_holder.val_bounds.horizontal;
		auto &vert_bounds = data_holder.val_bounds.vertical;
		const double local_ratio = (pos_ratio * src_bounds.horizontal.delta() - hor_bounds.low) / hor_bounds.delta();
		if (local_ratio < 0.0 || local_ratio > 1.0) return false;
		size_t column_index = std::round(local_ratio * (data_holder.size.horizontal) - 0.5); //Определяем индекс колонки
		column_index = qBound(0ui64, column_index, data_holder.size.horizontal - 1);
		Limits<double> ratio_vert_bounds = {
			(vert_bounds.low - src_bounds.vertical.low) / src_bounds.vertical.delta(),
			(vert_bounds.high - src_bounds.vertical.low) / src_bounds.vertical.delta()
		};
		if (ratio_vert_bounds.low < 0. || ratio_vert_bounds.high <= ratio_vert_bounds.low || ratio_vert_bounds.high > 1)
			return false;
		Limits<size_t> row_id_bounds = {
			size_t(std::llround(ratio_vert_bounds.low  * (passed_data.size() - 1))),
			size_t(std::llround(ratio_vert_bounds.high * (passed_data.size() - 1)))
		};
		SetDataToColumn(passed_data, row_id_bounds, column_index, data_holder);
		return true;
	};
	SetRatioToMatrix(spg_.base_data	   );
	SetRatioToMatrix(spg_.realtime_data);
    return true;
}

QPixmap & spg_core::SpgCore::GetRelevantPixmap(const ChartScaleInfo & scale_info)
{
    return renderer_.GetRelevantPixmap(scale_info);
}

spg_core::spg_data const & spg_core::SpgCore::GetSpectrogramInfo() const
{
    return spg_;
}

void spg_core::SpgCore::SetDataToColumn(const std::vector<float>& passed_data, Limits<size_t> row_id, size_t column_idx, spg_holder& passed_holder)
{
	if (passed_holder.relevant_vec[column_idx]) return; //if allready is relevant - do nothing
	const auto height = passed_holder.size.vertical;

	const double norm_koeff = double(row_id.delta()) / (height - 1);
	tbb::spin_mutex::scoped_lock guard_lock(spg_.rw_mutex_);

	for (int y = 0; y < height; y++)
	{
		const auto passed_id = row_id.low + double(y) * norm_koeff;
		passed_holder[y][column_idx] = passed_data[passed_id];
	}
	

	passed_holder.need_redraw = true;
	passed_holder.relevant_vec[column_idx] = true;
}


spg_core::SpgCore::~SpgCore()
{
}

bool spg_core::SpgCore::Emplace()
{

	auto emplace_holder = [&](spg_holder &holder_to_emplace, HV_Info<size_t> matrix_size) {
		holder_to_emplace.state = kNewData | kRequestStation;
		if (holder_to_emplace.val_bounds.horizontal.delta() <= 0) holder_to_emplace.val_bounds.horizontal = { 0, 1000 };              // Set x-axis Limits
		if (holder_to_emplace.val_bounds.vertical.delta() <= 0) holder_to_emplace.val_bounds.vertical = { 0.0, 1.0 };             // Set y-axis Limits
		auto& ref_size = holder_to_emplace.size;
		ref_size = matrix_size; 
		// Check for valid dimensions before resizing
		if (ref_size.vertical == 0 || ref_size.horizontal == 0) {
			std::cerr << "Error: Initial DPX data dimensions cannot be zero." << std::endl;
			return false;
		}

		const size_t data_size = static_cast<size_t>(ref_size.vertical) * ref_size.horizontal;  // Total number of data elements
		holder_to_emplace.data.clear();                      // Clear any existing data
		holder_to_emplace.data.resize(data_size, 0);         // Allocate and zero-initialize data
		holder_to_emplace.relevant_vec.assign(ref_size.horizontal, 0); 
		holder_to_emplace.need_redraw = true;                // Set redraw flag
		return true;
	};
	spg_.power_bounds = { 0, 100 };
	if (!emplace_holder(spg_.base_data, { 1024 * 2, 1024 * 1 }))
		return false;
	if (!emplace_holder(spg_.realtime_data, { 1024 * 1, 1024 / 2}))
		return false;
    return true;        // Initialization successful
}

void spg_core::SpgCore::Initialise(const freq_params & freq_params, const size_t samples_count)
{
}
