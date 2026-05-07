#include "TileDPX.h"
#include "GUI/Tools/gui_helper.h"
#include "GUI/Tools/gui_conversions.h"
void TileDPX::SetData(const draw_data & passed_info)
{
	if (passed_info.data.empty() || data_size_.hor == 0) return;

	tbb::spin_mutex::scoped_lock scoped_locker(mutex_);
	is_data_updated_ = true;

	PrepareForNewData(passed_info.freq_bounds);

	const auto& vis = val_bounds_.hor;
	const auto& full = passed_info.freq_bounds;
	
	// Границы пересечения
	double lo = std::max(full.low, vis.low);
	double hi = std::min(full.high, vis.high);
	if (lo >= hi) return;
	
	// Число точек данных в видимой области
	size_t n = passed_info.data.size();
	auto count = static_cast<size_t>(full.pos(hi) * (n - 1)) - static_cast<size_t>(full.pos(lo) * (n - 1));

	const double samples_per_pixel = count / data_size_.hor;
	if (samples_per_pixel < 1.0)
		DrawInterpolated(passed_info.data, passed_info.freq_bounds);
	else
		DrawOnlyPoints(passed_info.data, passed_info.freq_bounds);
}
void TileDPX::UpdateFromTile(const TileInterface::uptr& passed_data)
{
	auto* passed = dynamic_cast<TileDPX*>(passed_data.get());
	if (!passed || passed->data_size_.hor == 0 || passed->data_size_.vert == 0)
		return;

	for (size_t x_idx = 0; x_idx < data_size_.hor; ++x_idx) {
		if (column_weight[x_idx] != 0) continue;

		// 1. Находим мировое значение (src_val_x) для текущего x_idx
		double src_val_x = val_bounds_.hor.lerp(static_cast<double>(x_idx) / data_size_.hor);

		if (!passed->val_bounds_.hor.has_inside(src_val_x)) continue;

		// 2. Находим индекс в исходном буфере через pos()
		size_t sx = std::min(static_cast<size_t>(std::round(passed->val_bounds_.hor.pos(src_val_x) * passed->data_size_.hor)),
			passed->data_size_.hor - 1);

		size_t sum = 0;
		for (size_t y = 0; y < data_size_.vert; ++y) {
			double wy = val_bounds_.vert.lerp(static_cast<double>(y) / data_size_.vert);

			if (!passed->val_bounds_.vert.has_inside(wy)) continue;

			size_t sy = std::min(static_cast<size_t>(std::round(passed->val_bounds_.vert.pos(wy) * passed->data_size_.vert)),
				passed->data_size_.vert - 1);

			auto value = passed->data_[sy * passed->data_size_.hor + sx];
			data_[y * data_size_.hor + x_idx] = value;
			sum += value;
		}
		column_weight[x_idx] = sum;
	}
}
void TileDPX::UpdateQimage(dynamic_qimage & dyn_qimage, const Limits<double>& power_bounds)
{
	double  max_density = 0;
	double  summ_density = 0;
	int64_t	density_counter = 0;

	tbb::spin_mutex::scoped_lock scoped_locker(mutex_);

	const int grid_height = data_size_.vert;
	const int grid_width = data_size_.hor;
	argb_t *rgb_iter = dyn_qimage.data.data();
	for (int vert_number = 0; vert_number < grid_height; vert_number++)
	{
		const size_t* column_weight_iter = &column_weight[0]; //Reset for every row
		const auto matrix_idx_y = grid_height - vert_number - 1; //We have inversed image
		const int64_t* dpx_iter = &data_[matrix_idx_y * grid_width];
		for (int hor_number = 0; hor_number < grid_width; hor_number++)
		{
			const double column_weight = *(column_weight_iter++);
			const double density = column_weight ? *dpx_iter / column_weight : 0;
			dpx_iter++;
			argb_t color = GetNormColor(density);
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
	const auto new_density = density_counter ? summ_density / density_counter : 0.;
	if (new_density != last_average_density_) {
		last_average_density_ = new_density;
		is_data_updated_ = true;
	}

	last_max_density_ = max_density;
}

void TileDPX::Reset()
{	
	tbb::spin_mutex::scoped_lock scoped_locker(mutex_);

	data_.resize(data_size_.hor * data_size_.vert);
	column_weight.resize(data_size_.hor);
	ippsZero_8u((Ipp8u*)column_weight.data(), column_weight.size() * sizeof(column_weight[0]));
	relevant_vec_.assign(data_size_.hor, false);
}



void TileDPX::DrawOnlyPoints(const std::vector<float>& data,
	const Limits<double>& src_freq)
{
	const size_t width = static_cast<size_t>(data_size_.hor);
	const size_t height = static_cast<size_t>(data_size_.vert);

	const auto xb = val_bounds_.hor;
	const auto yb = val_bounds_.vert;

	const double freq_step =
		src_freq.delta() / static_cast<double>(data.size() - 1);

	for (size_t i = 0; i < data.size(); ++i)
	{
		const double x = src_freq.low + i * freq_step;
		const double y = data[i];

		if (!xb.has_inside(x) || !yb.has_inside(y))
			continue;

		size_t xi = static_cast<size_t>(xb.pos(x) * width);
		size_t yi = static_cast<size_t>(yb.pos(y) * height);

		if (xi == width)  --xi;
		if (yi == height) --yi;

		data_[yi * width + xi] += 1;
		column_weight[xi] += 1;
	}
}
void TileDPX::DrawInterpolated(const std::vector<float>& values,
	const Limits<double>& x_range)
{
	if (values.size() < 2) return;

	const auto& xb = val_bounds_.hor;
	const auto& yb = val_bounds_.vert;
	const size_t width = data_size_.hor, height = data_size_.vert;
	const double step = x_range.delta() / (values.size() - 1);

	for (size_t i = 0; i + 1 < values.size(); ++i) {
		const double x0 = x_range.low + i * step;
		const double x1 = x0 + step;

		if (!xb.has_inside(x0) && !xb.has_inside(x1)) continue;

		SlopeInterpolator interp(values[i], values[i + 1]);

		double p0 = xb.pos(x0) * width;
		double p1 = xb.pos(x1) * width;
		if (p0 > p1) std::swap(p0, p1);

		size_t x_begin = static_cast<size_t>(std::floor(p0));
		size_t x_end = static_cast<size_t>(std::ceil(p1));

		if (x_begin >= width) x_begin = width - 1;
		if (x_end > width)    x_end = width;
		if (x_end == 0 || x_begin >= width) continue;

		for (size_t px = x_begin; px < x_end; ++px) {
			const double x_world = xb.lerp(static_cast<double>(px) / width);
			double t = (x_world - x0) / step;
			if (t < 0.0) t = 0.0;
			else if (t > 1.0) t = 1.0;

			const double y_interp = interp.Interpolate(t);
			if (!yb.has_inside(y_interp)) continue;

			size_t py = static_cast<size_t>(std::round(yb.pos(y_interp) * height));
			if (py >= height) py = height - 1;

			data_[py * width + px] += 1;
			column_weight[px] += 1;
		}
	}
}
argb_t TileDPX::GetNormColor(const double relative_density) const
{
	const double normalized_density = qBound(0.0, relative_density / (last_average_density_ * 3), 1.0);
	return LUT_HSV_Instance::DensityToRGB(normalized_density);
}
void TileDPX::PrepareForNewData(const Limits<double>& freq_bounds)
{
	const size_t width = data_size_.hor;
	if (width == 0 || data_size_.vert == 0 || val_bounds_.hor.delta() <= 0)
		return;

	// 1. Используем pos() для получения относительных координат [0.0 ... 1.0]
	// 2. Умножаем на ширину, чтобы получить индексы
	int64_t x_begin = static_cast<int64_t>(std::floor(val_bounds_.hor.pos(freq_bounds.low) * width));
	int64_t x_end = static_cast<int64_t>(std::floor(val_bounds_.hor.pos(freq_bounds.high) * width));

	// Обрезка границ (заменяет проверку на пересечение и clamp)
	x_begin = std::max<int64_t>(0, x_begin);
	x_end = std::min<int64_t>(static_cast<int64_t>(width) - 1, x_end);

	if (x_begin > x_end)
		return;

	for (int64_t x = x_begin; x <= x_end; ++x) {
		if (relevant_vec_[x]) continue;

		// Обнуление столбца
		for (size_t y = 0; y < data_size_.vert; ++y) {
			data_[y * width + x] = 0;
		}

		column_weight[x] = 0;
		relevant_vec_[x] = true;
	}
}