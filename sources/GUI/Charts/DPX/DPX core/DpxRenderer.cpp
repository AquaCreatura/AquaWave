#include "DpxRenderer.h"
#include <algorithm>
#include <cmath>
#include <cstring> // Для memcpy
#include <stdexcept> // Для исключений

using namespace aqua_gui;

template<typename W>
W clamp_value_helper(W value, W min_val, W max_val) {
    return (value < min_val) ? min_val : (value > max_val) ? max_val : value;
}
dpx_core::DpxRenderer::DpxRenderer(dpx_data & init_val):
    dpx_(init_val)
{
	data_update_timer_.start();
}
QPixmap & dpx_core::DpxRenderer::GetRelevantPixmap(const ChartScaleInfo & scale_info)
{
	if (data_update_timer_.elapsed() >= 500) {
		UpdateDpxRgbData();
		data_update_timer_.restart();
	}
    
    const WH_Info<Limits<double>> &base_bounds   = scale_info.val_info_.min_max_bounds_;
    const WH_Info<Limits<double>> &target_bounds = scale_info.val_info_.cur_bounds;
    return zoomer_.GetPrecisedPart(base_bounds, target_bounds, scale_info.pix_info_.chart_size_px, false);

}
bool dpx_core::DpxRenderer::UpdateDpxRgbData()
{   
    bool need_redraw = dpx_.need_redraw;
    dpx_.need_redraw = false; //Сразу переводим в значение false, чтобы не пропустить новые данные, пока рисуем
    if(dpx_rgb_.size != dpx_.size)
    {
        dpx_rgb_.data.resize(dpx_.size.horizontal * dpx_.size.vertical);
        need_redraw = true;
        dpx_rgb_.qimage = QImage((uint8_t*)dpx_rgb_.data.data(), dpx_.size.horizontal, dpx_.size.vertical, QImage::Format::Format_ARGB32);
        dpx_rgb_.size = dpx_.size;
        zoomer_.SetNewBase(&dpx_rgb_.qimage);
    }
    if(need_redraw)
    {
		double  max_density = 0;
		double  summ_density = 0;
		int64_t	density_counter = 0;

        tbb::spin_mutex::scoped_lock scoped_locker(dpx_.redraw_mutex);
        //Здесь бы mutex по-хорошему
        const int grid_height = dpx_.size.vertical;
        const int grid_width  = dpx_.size.horizontal;
        argb_t *rgb_iter = dpx_rgb_.data.data();
        for(int column_iter = 0; column_iter < grid_height; column_iter++)
        {
            const int64_t* column_weight_iter = &dpx_.column_weight[0]; //Reset for every row
            const int64_t* dpx_iter           = dpx_[grid_height - column_iter - 1]; //We have inversed image
            for(int row_iter = 0; row_iter < grid_width; row_iter++)
            {
                const double column_weight = *(column_weight_iter++);
                const double density = column_weight ? *(dpx_iter++) / column_weight : 0;
                argb_t color = *GetNormalizedColor(density);
                *(rgb_iter++) = color;
				//Собираем статистические данные
				if(density > 0.)
				{
					max_density		=	std::max(max_density, density);
					summ_density	+= density;
					density_counter ++;
				}
            }
        }
		const auto new_density = summ_density / density_counter;
		if (new_density != last_average_density_) {
			last_average_density_ = new_density;
			dpx_.need_redraw = true;
		}
		
		last_max_density_	  = max_density;

        zoomer_.MarkForUpdate();
    }
    return true;
}
// Maps relative density to a color from the palette
const argb_t* dpx_core::DpxRenderer::GetNormalizedColor(double relative_density) const {
    const double normalized_density = clamp_value_helper(relative_density / (last_average_density_ * 3), 0.0, 1.0);
    // Validate palette size
    if ((relative_density == 0)) 
    {
        static const uint8_t default_color[4] = {0, 0, 0, 200}; // Default to black (little-endian)
        return (uint32_t*)(default_color);
    }

    argb_t* color_palette = LUT_HSV_Instance::get_table_ptr();
    // Calculate palette index and clamp within valid range
    int color_index = static_cast<int>(normalized_density * (hsv_table_size_c - 1));
    color_index = clamp_value_helper(color_index, 0, hsv_table_size_c - 1);

    // Return RGBA color from palette 
    return color_palette + color_index;
}