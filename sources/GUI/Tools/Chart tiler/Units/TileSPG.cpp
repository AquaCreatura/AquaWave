#include "TileSPG.h"

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

	auto src_range = make_range(val_bounds_.hor, src_bounds, src_data.size());
	if (src_range.delta() <= 0) return;

	auto dst_range = make_range(src_bounds, val_bounds_.hor, image_size_.hor);
	if (dst_range.delta() <= 0) return;

	SetDataToRow(
		src_data.data() + src_range.low,
		src_range.delta(),
		0,
		passed_draw.time_pos,
		dst_range
	);
}

void TileSPG::UpdateFromTile(const TileInterface::uptr & passed_data)
{
}

void TileSPG::UpdateQimage(dynamic_qimage & dyn_qimage)
{
}

void TileSPG::SetDataToRow(const float * passed_data, int data_size, const size_t row_idx, const double time_pos, const Limits<size_t> start_end_idx)
{
	if (relevant_vec[row_idx]) return; //if allready is relevant - do nothing
	tbb::spin_mutex::scoped_lock guard_lock(mutex_);

	const auto &img_width = image_size_.hor;

	const double norm_koeff = double(data_size) / (start_end_idx.delta() - 1);

	for (int x_idx = start_end_idx.low; x_idx < start_end_idx.high; x_idx++)
	{
		const auto passed_data_index = int(x_idx * norm_koeff);
		data_[row_idx * img_width + x_idx] = passed_data[passed_data_index];
	}

	is_data_updated_ = true;
	pos_vec[row_idx] = time_pos;
}
