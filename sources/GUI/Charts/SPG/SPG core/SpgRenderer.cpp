#include "SpgRenderer.h"
#pragma once
using namespace spg_core;

spg_core::SpgRenderer::SpgRenderer(spg_data & init_val): spg_(init_val)
{
	data_update_timer_.start();
}

QPixmap & spg_core::SpgRenderer::GetRelevantPixmap(const ChartScaleInfo & scale_info)
{
	const WH_Bounds<double> &base_bounds	= scale_info.val_info_.min_max_bounds_;
	const WH_Bounds<double> &target_bounds	= scale_info.val_info_.cur_bounds;

	if (IsModeSwitched(target_bounds) || data_update_timer_.elapsed() >= 500  ) {
		UpdateSpectrogramData();
		data_update_timer_.restart();
	}
    return zoomer_.GetPrecisedPart(last_val_bounds_, target_bounds, scale_info.pix_info_.chart_size_px);
}

bool spg_core::SpgRenderer::UpdateSpectrogramData()
{
    auto &holder_to_draw = realtime_mode_ ? spg_.realtime_data : spg_.base_data;

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
				if (density > 0) last_good_density = density;
				else if ((idx_power == 0) && (last_good_density != 0)) density = last_good_density;
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
			holder_to_draw.need_redraw = true;
		}

		last_max_density_ = max_density;  
		last_val_bounds_ = holder_to_draw.val_bounds;
        zoomer_.MarkForUpdate();
    }
    return true;
}


const argb_t * spg_core::SpgRenderer::GetNormalizedColor(double relative_density) const
{
	double delta = (last_max_density_ - last_average_density_[realtime_mode_]) * 0.5;
	const double normalized_density = qBound(0.0, (relative_density - last_average_density_[realtime_mode_]) / (delta),  1.0);
    // Validate palette size
    if ((normalized_density == 0)) 
    {
        static const uint8_t default_color[4] = {0, 0, 0, 0}; // Default to black (little-endian)
        return (uint32_t*)(default_color);
    }

    argb_t* color_palette = LUT_HSV_Instance::get_table_ptr();
    // Calculate palette index and clamp within valid range
    int color_index = static_cast<int>(normalized_density * (hsv_table_size_c - 1));
    color_index = qBound(0, color_index, hsv_table_size_c - 1);

    // Return RGBA color from palette 
    return color_palette + color_index;
}

bool spg_core::SpgRenderer::IsModeSwitched(WH_Bounds<double> realtime_size)
{
	auto &rt_data = spg_.realtime_data;
	bool has_old_data = rt_data.need_reset;
	if (rt_data.val_bounds != realtime_size) {
		tbb::spin_mutex::scoped_lock guard_lock(spg_.rw_mutex_);
		rt_data.val_bounds = realtime_size;
		std::fill(rt_data.relevant_vec.begin(), rt_data.relevant_vec.end(), false);
		rt_data.need_redraw = true;
		rt_data.need_reset = true; 
		has_old_data = true;
	}

	// Логика переключения НА базовый слой
	// Используем rt_data.need_reset напрямую, т.к. has_old_data была избыточна
	bool switch_to_base = realtime_mode_ && has_old_data;
	if (switch_to_base)
		spg_.base_data.need_redraw = true;

	// Логика переключения НА realtime
	bool switch_to_realtime = false;
	if (rt_data.station == HolderStation::FullOfData ||
		rt_data.station == HolderStation::ReadyToUse)
	{
		double scale = spg_.base_data.val_bounds.horizontal.delta() /
			rt_data.val_bounds.horizontal.delta();

		switch_to_realtime = (rt_data.size.horizontal >=
			spg_.base_data.size.horizontal / scale);
	}

	// Обновление итогового режима
	realtime_mode_ = switch_to_realtime && !switch_to_base;

	return switch_to_base;
}
