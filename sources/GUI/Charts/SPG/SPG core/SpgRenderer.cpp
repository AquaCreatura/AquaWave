#include "SpgRenderer.h"
#pragma once
using namespace spg_core;

spg_core::SpgRenderer::SpgRenderer(spg_data & init_val): spg_(init_val)
{
}

QPixmap & spg_core::SpgRenderer::GetRelevantPixmap(const ChartScaleInfo & scale_info)
{
    UpdateSpectrogramData();
    const WH_Info<Limits<double>> &base_bounds   = scale_info.val_info_.min_max_bounds_;
    const WH_Info<Limits<double>> &target_bounds = scale_info.val_info_.cur_bounds;
    return zoomer_.GetPrecisedPart(base_bounds, target_bounds, scale_info.pix_info_.chart_size_px, false);
}

bool spg_core::SpgRenderer::UpdateSpectrogramData()
{
    auto &basic =  spg_.base_data;
    bool need_redraw = basic.need_redraw;
    basic.need_redraw = false; //Сразу переводим в значение false, чтобы не пропустить новые данные, пока рисуем
    if(wrapper_rgb.size != basic.size)
    {
        wrapper_rgb.data.resize(basic.size.horizontal * basic.size.vertical);
        need_redraw = true;
        wrapper_rgb.qimage = QImage((uint8_t*)wrapper_rgb.data.data(), basic.size.horizontal, basic.size.vertical, QImage::Format::Format_ARGB32);
        wrapper_rgb.size = basic.size;
        zoomer_.SetNewBase(&wrapper_rgb.qimage);
    }
    if(need_redraw)
    {
        tbb::spin_mutex::scoped_lock guard_lock(spg_.rw_mutex_);
        //Здесь бы mutex по-хорошему
        const int grid_height = basic.size.vertical;
        const int grid_width  = basic.size.horizontal;
        argb_t *rgb_iter = wrapper_rgb.data.data();
        for(int y = 0; y < grid_height; y++)
        {
            const float* spg_iter = basic[grid_height - y - 1]; //We have inversed image
            for(int x = 0; x < grid_width; x++)
            {
                const double idx_power = *(spg_iter++);
                const double density = (idx_power - spg_.power_bounds.low) / spg_.power_bounds.delta();
                argb_t color = *GetNormalizedColor(density);

                *(rgb_iter++) = color;
            }
        }
        zoomer_.MarkForUpdate();
    }
    return true;
}

const argb_t * spg_core::SpgRenderer::GetNormalizedColor(double relative_density) const
{
    constexpr double MAX_THRESHOLD = 1; // Normalization threshold
    const double normalized_density = qBound(0.0, relative_density / MAX_THRESHOLD, 1.0);
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
