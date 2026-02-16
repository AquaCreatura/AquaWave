#include "SpgRenderer.h"
#pragma once
using namespace spg_core;

spg_core::SpgRenderer::SpgRenderer(spg_data & init_val): spg_(init_val)
{
	data_update_timer_.start();
}

QPixmap & spg_core::SpgRenderer::GetRelevantPixmap(const ChartScaleInfo & scale_info)
{
	const HorVerLim<double> &base_bounds	= scale_info.val_info_.min_max_bounds_;
	const HorVerLim<double> &target_bounds	= scale_info.val_info_.cur_bounds;

	if (IsModeSwitched(target_bounds) || data_update_timer_.elapsed() >= 500  ) {
		UpdateSpectrogramData();
		data_update_timer_.restart();
	}
    return zoomer_.GetPrecisedPart(last_val_bounds_, target_bounds, scale_info.pix_info_.chart_size_px);
}


bool spg_core::SpgRenderer::UpdateSpectrogramData()
{
    auto &holder_to_draw = realtime_mode_ ? spg_.realtime_data : spg_.base_data;
	bool is_rt_mode = realtime_mode_;
	if (is_rt_mode) {
		if (holder_to_draw.state & kPrecising) return true;
		if (holder_to_draw.state != kFullData && holder_to_draw.state != kReadyToUse) return false;
	}

    bool need_redraw = holder_to_draw.need_redraw;
    holder_to_draw.need_redraw = false; //Сразу переводим в значение false, чтобы не пропустить новые данные, пока рисуем
    if(wrapper_rgb.size != holder_to_draw.size)
    {
        wrapper_rgb.data.resize(holder_to_draw.size.horizontal * holder_to_draw.size.vertical);
        need_redraw = true;
        wrapper_rgb.qimage = QImage((uint8_t*)wrapper_rgb.data.data(), holder_to_draw.size.horizontal, holder_to_draw.size.vertical, QImage::Format::Format_ARGB32);
        wrapper_rgb.size = holder_to_draw.size;
        zoomer_.SetNewBase(&wrapper_rgb.qimage);
    }
    if(need_redraw)
    {
        tbb::spin_mutex::scoped_lock guard_lock(spg_.rw_mutex_);
        //Здесь бы mutex по-хорошему
        const int grid_height = holder_to_draw.size.vertical;
        const int grid_width  = holder_to_draw.size.horizontal;

		double  max_density = 0;
		double  summ_density = 0;
		int64_t	density_counter = 0;

        argb_t *rgb_iter = wrapper_rgb.data.data();
        for(int y = 0; y < grid_height; y++)
        {
            const float* spg_iter = holder_to_draw[grid_height - y - 1]; //We have inversed image
			double last_good_density = 0;
            for(int x = 0; x < grid_width; x++)
            {
                const double idx_power = *(spg_iter++);
                double density = (idx_power - spg_.power_bounds.low) / spg_.power_bounds.delta();
				if (holder_to_draw.relevant_vec[x]) last_good_density = density;
				else density = last_good_density;
                argb_t color = *GetNormalizedColor(density);
                *(rgb_iter++) = color;

				//Собираем статистические данные
				if (density > 0.)
				{
					max_density = std::max(max_density, density);
					summ_density += density;
					density_counter++;
				}
            }
        }
		const auto new_density = summ_density / density_counter;
		if (new_density != last_average_density_[realtime_mode_] ) {
			last_average_density_[realtime_mode_] = new_density;
			if (!realtime_mode_) last_average_density_[true] = last_average_density_[false];
			holder_to_draw.need_redraw = true;
		}

		last_max_density_[realtime_mode_] = max_density;
		last_val_bounds_ = holder_to_draw.val_bounds;
        zoomer_.MarkForUpdate();
    }
    return true;
}


const argb_t * spg_core::SpgRenderer::GetNormalizedColor(double relative_density) const
{
	double delta = (last_max_density_[realtime_mode_] - last_average_density_[realtime_mode_]) * 0.7;
	double normalized_density = (relative_density - last_average_density_[realtime_mode_]) / delta;
    // Validate palette size
    if ((normalized_density <= 0)) 
    {
        static const uint8_t default_color[4] = {0, 0, 0, 0}; // Default to black (little-endian)
        return (uint32_t*)(default_color);
    }
	normalized_density = qBound(0.0, normalized_density * normalized_density, 1.0);
    argb_t* color_palette = LUT_HSV_Instance::get_table_ptr();
    // Calculate palette index and clamp within valid range
    int color_index = static_cast<int>(normalized_density * (hsv_table_size_c - 1));
    color_index = qBound(0, color_index, hsv_table_size_c - 1);

    // Return RGBA color from palette 
    return color_palette + color_index;
}

bool spg_core::SpgRenderer::IsModeSwitched(HorVerLim<double> realtime_size)
{
	auto& rt_data = spg_.realtime_data;
	auto& base_data = spg_.base_data;

	const auto& passed_hor = realtime_size.horizontal;
	const auto& passed_vert = realtime_size.vertical;
	const auto& rt_hor = rt_data.val_bounds.horizontal;
	const auto& rt_vert = rt_data.val_bounds.vertical;

	const bool is_out_of_range = (passed_hor.low  < rt_hor.low || passed_hor.high > rt_hor.high) ||
		(passed_vert.low < rt_vert.low || passed_vert.high > rt_vert.high);

	constexpr double max_zoom = 1.05;
	const bool strong_focus = (rt_hor.delta()  > max_zoom * passed_hor.delta()) ||
		(rt_vert.delta() > max_zoom * passed_vert.delta());

	bool switch_to_base = realtime_mode_ && is_out_of_range;
	bool need_rt_mode = (rt_data.state & kReadyToUse) || (rt_data.state & kFullData) || (rt_data.state & kPrecising); //Обновляем Rt mode
	if (is_out_of_range || strong_focus) {
		if (is_out_of_range) need_rt_mode = false; //Если за диапазоном - делать нечего
		HorVerLim<double> new_size = realtime_size;

		// Apply shrinkage to fit within base bounds with margin
		double normalize_koeff = std::sqrt(max_zoom) * 0.98 / 2.0;
		const auto& src_bounds = base_data.val_bounds;

		new_size.horizontal.low = std::max(src_bounds.horizontal.low,
			passed_hor.mid() - passed_hor.delta() * normalize_koeff);
		new_size.horizontal.high = std::min(src_bounds.horizontal.high,
			passed_hor.mid() + passed_hor.delta() * normalize_koeff);
		new_size.vertical.low = std::max(src_bounds.vertical.low,
			passed_vert.mid() - passed_vert.delta() * normalize_koeff);
		new_size.vertical.high = std::min(src_bounds.vertical.high,
			passed_vert.mid() + passed_vert.delta() * normalize_koeff);

		int threshold_value = 0;
		int rt_threshold = 0;
		//Если ещё не переключились в режим rt - проверим базу
		if (is_out_of_range || !realtime_mode_) {
			const double hor_delta_base = base_data.val_bounds.horizontal.delta();
			const auto	 relative_start = base_data.val_bounds.horizontal.low;
			const double start_pos = (new_size.horizontal.low - relative_start) / hor_delta_base;
			const double end_pos = (new_size.horizontal.high - relative_start) / hor_delta_base;

			const int start_idx = static_cast<int>(start_pos * base_data.size.horizontal);
			const int end_idx = static_cast<int>(end_pos   * base_data.size.horizontal);

			for (int i = start_idx; i < end_idx; ++i) {
				threshold_value += base_data.relevant_vec[i];
			}
		}
		if (strong_focus && !is_out_of_range) {
			const double hor_delta_base = rt_data.val_bounds.horizontal.delta();
			const auto	 relative_start = rt_data.val_bounds.horizontal.low;
			const double start_pos = (new_size.horizontal.low - relative_start) / hor_delta_base;
			const double end_pos = (new_size.horizontal.high - relative_start) / hor_delta_base;

			const int start_idx = static_cast<int>(start_pos * rt_data.size.horizontal);
			const int end_idx = static_cast<int>(end_pos   * rt_data.size.horizontal);

			for (int i = start_idx; i < end_idx; ++i) {
				rt_threshold += rt_data.relevant_vec[i];
			}

			if (rt_threshold < threshold_value) {
				need_rt_mode = false;
			}


			rt_data.ready_threshold = rt_data.ready_threshold.load() * new_size.horizontal.delta() / rt_data.val_bounds.horizontal.delta();
			rt_data.ready_threshold = std::max(rt_data.ready_threshold.load(), rt_threshold);
		}

		// Update realtime data under lock
		{
			tbb::spin_mutex::scoped_lock guard(spg_.rw_mutex_);

			rt_data.val_bounds = std::move(new_size);
			rt_data.state = kRequestStation | (need_rt_mode ? kPrecising : kEmptyData);
			rt_data.ready_threshold = std::max(rt_threshold, threshold_value);
			std::fill(rt_data.relevant_vec.begin(), rt_data.relevant_vec.end(), false);
			rt_data.need_redraw = true;
		}
	}

	if (switch_to_base) {
		base_data.need_redraw = true;
	}

	realtime_mode_ = need_rt_mode;

	return switch_to_base;
}