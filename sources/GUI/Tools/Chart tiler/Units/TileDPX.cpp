#include "TileDPX.h"
#include "GUI/Tools/gui_helper.h"
#include "GUI/Tools/gui_conversions.h"
TileDPX::TileDPX()
{
	is_spg_ = false;
	max_column_weight_		= 200'000;
	max_trans_column_weight_= 20'000;
	trans_decrease_counter_	= 0;

}
void TileDPX::SetData(const draw_data & passed_info)
{
	if (passed_info.data.empty() || data_size_.hor == 0) return;

	is_data_updated_ = true;

	PrepareForNewData();

	const auto& vis = val_bounds_.hor;
	const auto& full = passed_info.freq_bounds;
	
	// Границы пересечения
	double lo = std::max(full.low, vis.low);
	double hi = std::min(full.high, vis.high);
	if (lo >= hi) return;
	
	// Число точек данных в видимой области
	size_t n = passed_info.data.size();
	auto count = static_cast<int64_t>(full.pos(hi) * (n - 1)) - static_cast<int64_t>(full.pos(lo) * (n - 1));

	const double samples_per_pixel = double(count) / data_size_.hor;
	if (samples_per_pixel < 1.0)
		DrawInterpolated(passed_info.data, passed_info.freq_bounds);
	else
		DrawOnlyPoints(passed_info.data, passed_info.freq_bounds);
}
void TileDPX::UpdateFromTile(const TileInterface* passed_data)
{
	auto* passed = dynamic_cast<const TileDPX*>(passed_data);
	if (!passed || passed->data_size_.hor == 0 || passed->data_size_.vert == 0 || passed_data->val_bounds_.hor.delta() == 0)
		return;
	double hor_ratio = passed_data->val_bounds_.hor.delta() / val_bounds_.hor.delta();
	const bool is_relevant = hor_ratio <= 1.;
	int64_t new_max_trans_density = 0;
	for (size_t x_idx = 0; x_idx < data_size_.hor; ++x_idx) {
		if (column_weight[x_idx] != 0) continue;
		if (passed->column_weight[x_idx] == 0) continue;
		// 1. Находим мировое значение (src_val_x) для текущего x_idx
		double src_val_x = val_bounds_.hor.lerp((x_idx + 0.5) / data_size_.hor);
		if (!passed->val_bounds_.hor.has_inside(src_val_x)) continue;

		// 2. Находим индекс в исходном буфере через pos()
		size_t sx = std::min(static_cast<size_t>(std::round(passed->val_bounds_.hor.pos(src_val_x) * passed->data_size_.hor - 0.5)),
			passed->data_size_.hor - 1);
		double norm_koeff = double(max_trans_column_weight_) /  std::max(passed->column_weight[x_idx], max_trans_column_weight_);

		size_t sum = 0;
		for (size_t y = 0; y < data_size_.vert; ++y) {
			double wy = val_bounds_.vert.lerp(static_cast<double>(y + 0.5) / data_size_.vert);

			if (!passed->val_bounds_.vert.has_inside(wy)) continue;

			size_t sy = std::min(static_cast<size_t>(std::round(passed->val_bounds_.vert.pos(wy) * passed->data_size_.vert - 0.5)),
				passed->data_size_.vert - 1);

			auto cur_val = passed->data_[sy * passed->data_size_.hor + sx];
			cur_val = std::ceil(cur_val * norm_koeff); //Нормируем
			new_max_trans_density = std::max<int64_t>(new_max_trans_density, cur_val);
			data_[y * data_size_.hor + x_idx] = cur_val;
			sum += cur_val;
		}
		column_weight[x_idx] = sum;
		//relevant_vec_[x_idx] = is_relevant;
	}
	if(new_max_trans_density)
		trans_decrease_counter_ = new_max_trans_density;

	is_data_updated_ = true;
	if (is_relevant || (last_average_density_ == 0.0f)) {
		last_average_density_ = passed_data->last_average_density_;
	}
}
void TileDPX::UpdateQimage(dynamic_qimage & dyn_qimage, const Limits<double>& power_bounds)
{
	double  max_density = 0;
	double  summ_density = 0;
	int64_t	density_counter = 0;

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
}

void TileDPX::Reset()
{	
	data_.resize(data_size_.hor * data_size_.vert, 0);
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

		int64_t x_begin = static_cast<int64_t>(std::floor(p0));
		int64_t x_end = static_cast<int64_t>(std::ceil(p1));

		if (x_begin >= width) x_begin = width - 1;
		if (x_end > width)    x_end = width;
		if (x_end == 0 || x_begin >= width) continue;

		for (size_t px = x_begin; px < x_end; ++px) {
			const double x_world = xb.lerp((px + 0.5) / width);
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

void TileDPX::PrepareForNewData()
{
	const int width	= data_size_.hor;
	const int height = data_size_.vert;

	if (width == 0 || height == 0 || val_bounds_.hor.delta() <= 0)
		return;
	const bool is_trans = trans_decrease_counter_ > 0;
	const double max_deviation = 0.15;
	const int64_t max_column_size = is_trans ? max_trans_column_weight_ : max_column_weight_;

	// Сначала смотрим - а надо ли вообще что-то делать
	{
		bool is_all_relevant = true;
		bool small_values = false;
		for (int64_t x = 0; x < width; x++) {
			if (relevant_vec_[x]) continue;
			is_all_relevant = false;
			if (column_weight[x] < max_column_size*(1 + max_deviation)) {
				small_values = true;
				break;
			}
		}
		//Если нет необходимости суетиться - выходим
		if (is_all_relevant || small_values) {
			return;
		}
	}
	//Нормируем, приводим к среднему значению
	{
		double trans_norm_koeff = 0.01;
		for (int64_t x = 0; x < width; x++) {
			if (relevant_vec_[x]) continue;
			int cur_col_decrease = 0;
			const double norm_koeff = double(std::min<int64_t>(column_weight[x], max_column_size)) / column_weight[x];
			trans_norm_koeff = std::max(trans_norm_koeff, norm_koeff);
			size_t new_column_weight = 0;
			for (size_t y = 0; y < data_size_.vert; ++y) {
				const int64_t new_weight = data_[y * width + x] * norm_koeff;
				new_column_weight += new_weight;
				data_[y * width + x] = new_weight;
			}
			column_weight[x] = new_column_weight;
		}
		trans_decrease_counter_ *= trans_norm_koeff;
		is_data_updated_ = true;
	}
}