#include "TileDPX.h"
#include "GUI/Tools/gui_helper.h"
#include "GUI/Tools/gui_conversions.h"
void TileDPX::SetData(const draw_data & passed_info)
{
	auto &draw_vec = passed_info.data;
	if (draw_vec.empty())
		return;
	
	tbb::spin_mutex::scoped_lock scoped_locker(mutex_);
	if (data_size_.hor == 0) {
		return;
	}
	is_data_updated_ = true;
	
	const double samples_per_pixel = double(draw_vec.size()) / data_size_.hor;
	if (samples_per_pixel < 1.) {
		return DrawInterpolated(draw_vec, passed_info.freq_bounds);
	}
	else {
		return DrawOnlyPoints(draw_vec, passed_info.freq_bounds);

	}
}

void TileDPX::UpdateFromTile(const TileInterface::uptr & passed_data)
{
}

void TileDPX::UpdateQimage(dynamic_qimage& dyn_qimage, const Limits<double>&)
{
    double max_density = 0.0;
    double sum_density = 0.0;
    int64_t count = 0;

    tbb::spin_mutex::scoped_lock lock(mutex_);

    const int h = data_size_.vert;
    const int w = data_size_.hor;

    argb_t* rgb = dyn_qimage.data.data();

    for (int y = 0; y < h; ++y)
    {
        const int row = (h - 1 - y) * w;
        const int64_t* dpx = &data_[row];
        const size_t* weights = column_weight.data();

        for (int x = 0; x < w; ++x)
        {
            const double density = weights[x] ? dpx[x] / weights[x] : 0.0;

            *rgb++ = GetNormColor(density);

            if (density > 0.0)
            {
                max_density = std::max(max_density, density);
                sum_density += density;
                ++count;
            }
        }
    }

    const double avg = count ? sum_density / count : 0.0;

    if (avg != last_average_density_)
    {
        last_average_density_ = avg;
        is_data_updated_ = true;
    }

    last_max_density_ = max_density;
}


void TileDPX::DrawOnlyPoints(const std::vector<float>& data, const Limits<double>& x_limits)
{
	// Входные границы по X
	const double x_in_min = x_limits.low;
	const double x_in_max = x_limits.high;

	// Размер сетки (в пикселях)
	const size_t width = static_cast<size_t>(data_size_.hor);
	const size_t height = static_cast<size_t>(data_size_.vert);

	// Границы данных по X
	auto& x_val_bounds = val_bounds_.hor;
	const double x_min = x_val_bounds.low;
	const double x_range = x_val_bounds.delta();
	const double x_step_val = x_range / static_cast<double>(width);

	// Границы данных по Y
	auto& y_bounds = val_bounds_.vert;
	const double y_min = y_bounds.low;
	const double y_step_val = y_bounds.delta() / height;
	const double y_step_inv = 1.0 / y_step_val;

	// Шаг входных данных по X
	const double x_in_range = x_in_max - x_in_min;
	const double x_in_step = x_in_range / static_cast<double>(data.size() - 1);

	// Проход по входным точкам
	for (size_t i = 0; i < data.size(); ++i)
	{
		// Текущий X
		const double x_val = x_in_min + static_cast<double>(i) * x_in_step;

		// Индекс по X
		size_t xi = static_cast<size_t>((x_val - x_min) / x_step_val);

		if (xi >= width) {
			if (xi == width && x_val == x_val_bounds.high) {
				xi = width - 1;
			}
			else {
				continue;
			}
		}

		// Текущее значение Y
		const double y_val = static_cast<double>(data[i]);

		// Индекс по Y
		size_t yi = static_cast<size_t>((y_val - y_min) * y_step_inv);

		if (yi >= height) {
			if (yi == height && y_val == y_bounds.high) {
				yi = height - 1;
			}
			else {
				continue;
			}
		}

		// Запись в буфер
		data_[yi * width + xi] += 1;
		column_weight[xi] += 1;
	}
}

void TileDPX::DrawInterpolated(const std::vector<float>& vals, const Limits<double>& x_lim)
{
	// Нужно минимум 2 точки
	const int64_t n = vals.size();
	if (n < 2) {
		return;
	}

	// Диапазон и шаг входа
	const double x_range = static_cast<double>(x_lim.high - x_lim.low);
	const double x_step = x_range / (n - 1);

	// Границы сетки
	const auto& x_lim_dpx = val_bounds_.hor;
	const auto& y_lim_dpx = val_bounds_.vert;

	// Размер сетки
	const int64_t w = static_cast<int64_t>(data_size_.hor);
	const int64_t h = static_cast<int64_t>(data_size_.vert);

	// Параметры сетки по X
	const double x0_dpx = x_lim_dpx.low;
	const double x_bin_w = x_lim_dpx.delta() / static_cast<double>(w);

	// Параметры сетки по Y
	const double y0_dpx = y_lim_dpx.low;
	const double y_step = y_lim_dpx.delta() / static_cast<double>(h);
	const double inv_y_step = 1.0 / y_step;

	// Проход по сегментам
	for (int64_t i = 0; i < n - 1; ++i) {
		const SlopeInterpolator interp(vals[i], vals[i + 1]);

		// Границы сегмента
		const double x0 = x_lim.low + i * x_step;
		const double x1 = x_lim.low + (i + 1) * x_step;

		// Диапазон колонок
		int64_t ix0 = static_cast<int64_t>((x0 - x0_dpx) / x_bin_w);
		int64_t ix1 = static_cast<int64_t>((x1 - x0_dpx) / x_bin_w);

		if (x1 > x0) {
			ix1 = static_cast<int64_t>(std::ceil((x1 - x0_dpx) / x_bin_w));
		}
		if (ix0 > w || ix1 < 0)
			continue;

		size_t bx0 = static_cast<size_t>(std::max(int64_t(0), ix0));
		size_t bx1 = static_cast<size_t>(std::min(static_cast<int64_t>(w), ix1 + 1));

		if (w > 0 && bx0 >= w) bx0 = w - 1;
		if (w > 0 && bx1 > w)  bx1 = w;

		// Заполнение
		for (size_t bx = bx0; bx < bx1; ++bx) {
			const double xc = x0_dpx + static_cast<double>(bx) * x_bin_w;

			// Параметр вдоль сегмента
			double t = (xc - x0) / x_step;
			if (t < 0.0) t = 0.0;
			if (t > 1.0) t = 1.0;

			const double y = interp.Interpolate(t);

			// Индекс строки
			double iyf = std::round((y - y0_dpx) * inv_y_step);
			if (iyf < 0 || iyf >= h) continue;

			size_t by = static_cast<size_t>(iyf);

			
			data_[by * w + bx] += 1;
			column_weight[bx] += 1;
			
		}
	}
}

argb_t TileDPX::GetNormColor(const double relative_density) const
{
	const double normalized_density = qBound(0.0, relative_density / (last_average_density_ * 3), 1.0);
	return LUT_HSV_Instance::DensityToRGB(normalized_density);
}
