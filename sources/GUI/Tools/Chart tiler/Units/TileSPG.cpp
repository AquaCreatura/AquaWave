#include "TileSPG.h"
#include "GUI/Tools/gui_conversions.h"
TileSPG::TileSPG() {
	is_rotated_ = true;
}

void TileSPG::SetData(const draw_data & passed_draw)
{
	const auto& src_bounds = passed_draw.freq_bounds;
	const auto& src_data = passed_draw.data;
	
	auto project = [](double value, const Limits<double>& src, size_t dst_size)
	{
		return (value - src.low) / src.delta() * (dst_size - 1);
	};

	auto clamp_index = [](double v, size_t max_idx)
	{
		long r = std::lround(v);
		r = std::clamp(r, 0L, static_cast<long>(max_idx));
		return static_cast<size_t>(r);
	};

	// Универсальная лямбда для получения диапазона
	auto make_range = [&](const Limits<double>& from,
		const Limits<double>& to,
		size_t to_size) -> Limits<size_t>
	{
		double start = std::max(0.0, project(from.low, to, to_size));
		double end = std::min(double(to_size - 1), project(from.high, to, to_size));
		return Limits<size_t>{
			clamp_index(start, to_size - 1),
				clamp_index(end, to_size - 1)
		};
	};
	tbb::spin_mutex::scoped_lock guard_lock(mutex_);

	auto src_range = make_range(val_bounds_.hor, src_bounds, src_data.size());
	if (src_range.delta() <= 0) return;

	auto dst_range = make_range(src_bounds, val_bounds_.hor, data_size_.hor);
	if (dst_range.delta() <= 0) return;

	const double pos_ratio = (passed_draw.time_pos - val_bounds_.vert.low) / val_bounds_.vert.delta();
	const int res_idx = static_cast<int>(std::round(pos_ratio - 0.5));
	if (res_idx < val_bounds_.vert.low || res_idx > val_bounds_.vert.high)
		return;
	if (relevant_vec_[res_idx]) 
		return; 
	SetDataToRow( src_data.data() + src_range.low, src_range.delta(), res_idx, dst_range);
	relevant_vec_[res_idx] = true;
	pos_vec_[res_idx] = pos_ratio;
}

void TileSPG::UpdateFromTile(const TileInterface::uptr & passed_data)
{
}

void TileSPG::UpdateQimage(dynamic_qimage & dyn_qimage, const Limits<double> &power_bounds)
{
	if (is_rotated_)
		return UpdateQimageRotate(dyn_qimage, power_bounds);

	tbb::spin_mutex::scoped_lock guard_lock(mutex_);

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
			double density = (idx_power - power_bounds.low) / power_bounds.delta();
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
	last_max_density_ = max_density;
}

void TileSPG::UpdateQimageRotate(dynamic_qimage & dyn_qimage, const Limits<double>& power_bounds)
{
	tbb::spin_mutex::scoped_lock guard_lock(mutex_);

	const int src_height = data_size_.vert;
	const int src_width = data_size_.hor;

	const int dst_height = dyn_qimage.size.vert;
	const int dst_width = dyn_qimage.size.hor;

	double  max_density = 0;
	double  summ_density = 0;
	int64_t	density_counter = 0;

	float *src_data = data_.data();
	argb_t *dst_data = dyn_qimage.data.data();
	for (int dst_x = 0; dst_x < dst_width; dst_x++)
	{
		double last_good_density = 0;
		for (int dst_y = 0; dst_y < dst_height; dst_y++)
		{
			const int src_x = dst_height - dst_y;
			const int src_y = dst_x;
			const double idx_power = src_data[src_y * src_width + src_x];
			double density = (idx_power - power_bounds.low) / power_bounds.delta();
			if (relevant_vec_[dst_x]) last_good_density = density;
			else density = last_good_density;
			argb_t color = GetNormColor(density);

			*(dst_data++) = color;			
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
	last_max_density_ = max_density;
}

void TileSPG::SetDataToRow(const float * passed_data, int data_size, const size_t row_idx, const Limits<size_t> dst_bounds)
{
	const auto &img_width = data_size_.hor;

	const double norm_koeff = double(data_size) / (dst_bounds.delta() - 1);

	for (int x_idx = dst_bounds.low; x_idx < dst_bounds.high; x_idx++)
	{
		const auto passed_data_index = int(x_idx * norm_koeff);
		data_[row_idx * img_width + x_idx] = passed_data[passed_data_index];
	}
	is_data_updated_ = true;
}

argb_t TileSPG::GetNormColor(double relative_density) const
{
	double delta = (last_max_density_ * 0.7 - last_average_density_);
	double normalized_density = (relative_density - last_average_density_) / delta;
	return LUT_HSV_Instance::DensityToRGB(normalized_density);
}
