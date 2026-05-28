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

	SetDataToRow(src_data.data() + passed_cut_range.low, passed_cut_range.delta(), tile_draw_bounds, passed_draw.time_pos, true);
}
void TileSPG::UpdateFromTile(const TileInterface* passed_tile) {
	const auto* src = static_cast<const TileSPG*>(passed_tile);

	// Вспомогательная функция: значение -> индекс в сетке
	auto to_index = [](const Limits<double>& bounds, size_t size, double value) -> size_t {
		long idx = static_cast<long>(std::floor(bounds.pos(value) * size));
		return static_cast<size_t>(std::clamp(idx, 0L, static_cast<long>(size - 1)));
	};
	bool is_good_density = false;
	const bool is_relevant = passed_tile->val_bounds_.hor == val_bounds_.hor;
	for (size_t src_row = 0; src_row < src->data_size_.vert; ++src_row) {
		if (!src->pos_vec_[src_row] < 0.f) continue;

		double time = src->pos_vec_[src_row];
		if (!val_bounds_.vert.has_inside(time)) continue;

		size_t dst_row = static_cast<size_t>(std::floor(val_bounds_.vert.pos(time) * data_size_.vert));
		if (pos_vec_[dst_row] > 0.f) continue;

		// Пересечение частотных диапазонов
		Limits<double> overlap{
			std::max(val_bounds_.hor.low, src->val_bounds_.hor.low),
			std::min(val_bounds_.hor.high, src->val_bounds_.hor.high)
		};
		if (overlap.delta() <= 0) continue;
		is_good_density = true;
		// Диапазоны индексов в исходных и целевых данных (high эксклюзивный)
		Limits<size_t> src_indices{
			to_index(src->val_bounds_.hor, src->data_size_.hor, overlap.low),
			to_index(src->val_bounds_.hor, src->data_size_.hor, overlap.high) + 1
		};
		Limits<size_t> dst_indices{
			to_index(val_bounds_.hor, data_size_.hor, overlap.low),
			to_index(val_bounds_.hor, data_size_.hor, overlap.high) + 1
		};

		const float* src_data_ptr = src->data_.data() + src_row * src->data_size_.hor + src_indices.low;
		SetDataToRow(src_data_ptr, src_indices.delta(), dst_indices, time, is_relevant);
	}
	if (is_good_density && last_average_density_ == 1.0f)
		last_average_density_ = src->last_average_density_;
}

void TileSPG::UpdateQimage(dynamic_qimage & dyn_qimage, const Limits<double> &power_bounds)
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
			const int base_x_freq = passed_height - passed_y;
			const int base_y_time = passed_x;
			const double idx_power = src_data[base_y_time * src_width + base_x_freq];
			double density = idx_power / 100.;  (idx_power - power_bounds.low) / power_bounds.delta();
			if (pos_vec_[base_y_time] >= 0.f) last_good_density = density;
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


void TileSPG::SetDataToRow(const float * passed_data, int data_size, const Limits<size_t> dst_bounds, double time_pos, const bool is_relevant)
{
	const auto &img_width = data_size_.hor;
	const double pos_ratio = val_bounds_.vert.pos(time_pos);
	const int res_idx = std::floor(pos_ratio * data_size_.vert);

	// Проверка попадания в row и флаг актуальности
	if (!val_bounds_.vert.has_inside(time_pos) || res_idx < 0 || relevant_vec_[res_idx])
		return;

	const double norm_koeff = double(data_size) / (dst_bounds.delta() - 1);
	float* raw_ptr = &data_[res_idx * img_width];
	ippsZero_32f(raw_ptr, img_width);
	for (int x_idx = dst_bounds.low; x_idx < dst_bounds.high; x_idx++)
	{
		const auto passed_data_index = int((x_idx - dst_bounds.low) * norm_koeff);
		raw_ptr[x_idx] = passed_data[passed_data_index];
	}
	is_data_updated_ = true;
	relevant_vec_[res_idx] = is_relevant;
	pos_vec_[res_idx] = time_pos;
}

argb_t TileSPG::GetNormColor(double relative_density) const
{
	double delta = (last_average_density_ * 0.7);
	double normalized_density = (relative_density - last_average_density_) / delta;
	return LUT_HSV_Instance::DensityToRGB(normalized_density);
}

void TileSPG::Reset()
{
	data_.resize(data_size_.hor * data_size_.vert, 0);
	relevant_vec_.assign(data_size_.vert, false);
	//pos_vec_.resize(data_size_.vert);
	pos_vec_.assign(data_size_.vert, -1.);
}


