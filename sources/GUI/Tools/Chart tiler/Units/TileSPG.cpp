#include "TileSPG.h"
#include "GUI/Tools/gui_conversions.h"
TileSPG::TileSPG() {
	is_spg_ = true;
	is_rotated_ = true;
}

void TileSPG::SetData(const draw_data & passed_draw)
{
	const auto& passed_freq = passed_draw.freq_bounds;
	const auto& src_data = passed_draw.data;
	//По вертикали время, по горизонтале - частота
	//Проецируем диапазон мировых координат на диапазон данных
	auto make_range = [](const Limits<double>& from, const Limits<double>& to, size_t to_size) -> Limits<size_t> {
		auto to_idx = [&](double val) {
			long idx = static_cast<long>(std::floor(to.pos(val) * to_size));
			return static_cast<size_t>(std::clamp(idx, 0L, static_cast<long>(to_size - 1)));
		};
		return { to_idx(from.low), to_idx(from.high) };
	};
	//Какая часть от диапазона входных данных лежит внутри нашего тайла (проецируем диапазон тайла на данные)
	auto passed_cut_range = make_range(val_bounds_.hor, passed_freq, src_data.size());
	//Какая часть от диапазона тайла лежит внутри диапазона входных данных (проецируем входной диапазон на тайлы - обычно 100%)
	auto tile_draw_bounds = make_range(passed_freq, val_bounds_.hor, data_size_.hor);

	// delta() == 0 сразу отсекает пустые или некорректные диапазоны
	if (passed_cut_range.delta() == 0 || tile_draw_bounds.delta() == 0) return;

	const double pos_ratio = val_bounds_.vert.pos(passed_draw.time_pos);
	const int res_idx = std::floor(pos_ratio * data_size_.vert);

	// Проверка попадания в row и флаг актуальности
	if (!val_bounds_.vert.has_inside(passed_draw.time_pos) || res_idx < 0 || relevant_vec_[res_idx])
		return;

	SetDataToRow(src_data.data() + passed_cut_range.low, passed_cut_range.delta(), res_idx, tile_draw_bounds);
	relevant_vec_[res_idx] = true;
	pos_vec_[res_idx] = pos_ratio;
}
void TileSPG::UpdateFromTile(const TileInterface* passed_data)
{
}

void TileSPG::UpdateQimage(dynamic_qimage & dyn_qimage, const Limits<double> &power_bounds)
{
	if (is_rotated_)
		return UpdateQimageRotate(dyn_qimage, power_bounds);


	//Здесь бы mutex по-хорошему
	const int src_height = data_size_.vert;
	const int src_width = data_size_.hor;

	double  max_density = 0;
	double  summ_density = 0;
	int64_t	density_counter = 0;

	argb_t *rgb_iter = dyn_qimage.data.data();
	for (int src_y = 0; src_y < src_height; src_y++)
	{
		float *spg_iter = &data_[src_y * src_width]; 
		double last_good_density = 0;
		for (int src_x = 0; src_x < src_width; src_x++)
		{
			const double idx_power = *(spg_iter++);
			double density = idx_power;  (idx_power - power_bounds.low) / power_bounds.delta();
			if (relevant_vec_[src_y]) last_good_density = density;
			else density = last_good_density;
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
	const auto new_density = summ_density / density_counter;
	if (new_density != last_average_density_) {
		last_average_density_ = new_density;
		is_data_updated_ = true;
	}

}

void TileSPG::UpdateQimageRotate(dynamic_qimage & dyn_qimage, const Limits<double>& power_bounds)
{
	const int src_height = data_size_.vert;
	const int src_width = data_size_.hor;

	const int passed_height = dyn_qimage.size.vert;
	const int passed_width = dyn_qimage.size.hor;

	double  max_density = 0;
	double  summ_density = 0;
	int64_t	density_counter = 0;

	float *src_data = data_.data();
	argb_t *qimage_ptr = dyn_qimage.data.data();
	for (int passed_y = 0; passed_y < passed_height; passed_y++)
	{
		double last_good_density = 0;
		for (int passed_x = 0; passed_x < passed_width; passed_x++)
		{
			const int src_x_freq = passed_height - passed_y;
			const int src_y_time = passed_x;
			const double idx_power = src_data[src_y_time * src_width + src_x_freq];
			double density = idx_power / 100.;  (idx_power - power_bounds.low) / power_bounds.delta();
			if (relevant_vec_[passed_x]) last_good_density = density;
			else density = last_good_density;
			argb_t color = GetNormColor(density);

			*(qimage_ptr++) = color;			
			//Собираем статистические данные
			if (density > 0.)
			{
				max_density = std::max(max_density, density);
				summ_density += density;
				density_counter++;
			}
		}
	}
	if (density_counter == 0) 
		return;
	const auto new_density = summ_density / density_counter;
	if (new_density != last_average_density_) {
		last_average_density_ = new_density;
		is_data_updated_ = true;
	}
}

void TileSPG::SetDataToRow(const float * passed_data, int data_size, const size_t row_idx, const Limits<size_t> dst_bounds)
{
	const auto &img_width = data_size_.hor;

	const double norm_koeff = double(data_size) / (dst_bounds.delta() - 1);
	float* raw_ptr = &data_[row_idx * img_width];
	for (int x_idx = dst_bounds.low; x_idx < dst_bounds.high; x_idx++)
	{
		const auto passed_data_index = int(x_idx * norm_koeff);
		raw_ptr[x_idx] = passed_data[passed_data_index];
	}
	is_data_updated_ = true;
}

argb_t TileSPG::GetNormColor(double relative_density) const
{
	double delta = (last_average_density_ * 2);
	double normalized_density = (relative_density - 0 * last_average_density_) / delta;
	return LUT_HSV_Instance::DensityToRGB(normalized_density);
}

void TileSPG::Reset()
{
	data_.resize(data_size_.hor * data_size_.vert, 0);
	relevant_vec_.assign(data_size_.vert, false);
	pos_vec_.resize(data_size_.vert);
}


