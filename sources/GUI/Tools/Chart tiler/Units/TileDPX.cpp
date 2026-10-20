#include "TileDPX.h"
#include "GUI/Tools/gui_worker.h"
#include "GUI/Tools/gui_conversions.h"

// Конструктор
TileDPX::TileDPX(const bool need_decay)
{
	is_spg_ = false;
	trans_decrease_counter_ = 0;
	need_decay_ = need_decay;
	UpdateDecayInfo();
}

// ------------------------------------------------------------------
// SetData – добавление новых данных (без немедленного затухания)
// ------------------------------------------------------------------
void TileDPX::SetData(const draw_data& passed_info)
{
	if (passed_info.data.empty() || data_size_.hor == 0) return;
	periodical_block_counter_++;
	is_data_updated_ = true;

	const auto& vis = val_bounds_.hor;
	const auto& full = passed_info.freq_bounds;

	double lo = std::max(full.low, vis.low);
	double hi = std::min(full.high, vis.high);
	if (lo >= hi) return;

	size_t n = passed_info.data.size();
	auto count = static_cast<int64_t>(full.pos(hi) * (n - 1)) - static_cast<int64_t>(full.pos(lo) * (n - 1));
	const double samples_per_pixel = double(count) / data_size_.hor;

	if (samples_per_pixel < 1.0)
		DrawInterpolated(passed_info.data, passed_info.freq_bounds);
	else
		DrawOnlyPoints(passed_info.data, passed_info.freq_bounds);

	last_samples_per_pixel_ = samples_per_pixel;
}

// ------------------------------------------------------------------
// UpdateFromTile – заполнение пустых колонок из вышестоящего тайла
// ------------------------------------------------------------------
void TileDPX::UpdateFromTile(const TileInterface* passed_data)
{
	auto* casted_past = dynamic_cast<const TileDPX*>(passed_data);
	if (!casted_past) return;
	if(casted_past->data_size_.hor == 0 || casted_past->data_size_.vert == 0 || passed_data->val_bounds_.hor.delta() == 0)
		return;
	// 1. Применяем затухание к текущим данным, чтобы они «постарели»
	ApplyDecay();
	// 2. Коэффициент масштабирования при переходе между уровнями детализации
	double hor_ratio = passed_data->val_bounds_.hor.delta() / val_bounds_.hor.delta();


	
	size_t max_col_size = 0;
	for (auto it : column_weight_vec_) 
		max_col_size = std::max(it, max_col_size);
	for (auto it : casted_past->column_weight_vec_)
		max_col_size = std::max(it, max_col_size);

	const size_t trans_column_size = std::min<size_t>(max_col_size, 200'000); // 

	int64_t new_max_trans_density = 0;

	for (size_t x_idx_base = 0; x_idx_base < data_size_.hor; ++x_idx_base) {
		// Заполняем только те колонки, которые полностью пусты (вес == 0)
		if (column_weight_vec_[x_idx_base] != 0)
		{
			double norm_koeff = double(trans_column_size) / column_weight_vec_[x_idx_base]; 
			if (norm_koeff > 0.85 && norm_koeff < 1.0)
				continue;
			size_t new_summ = 0;
			for (size_t y = 0; y < data_size_.vert; ++y) {
				data_[y * data_size_.hor + x_idx_base] *= norm_koeff;
				new_summ += data_[y * data_size_.hor + x_idx_base];
			}
			column_weight_vec_[x_idx_base] = new_summ;
			continue;
		}

		// Мировое значение X для текущего пикселя
		double world_x = val_bounds_.hor.lerp((x_idx_base + 0.5) / data_size_.hor);
		if (!casted_past->val_bounds_.hor.has_inside(world_x)) continue;

		// Индекс в буфере вышестоящего тайла
		size_t x_idx_passed = static_cast<size_t>(std::round(casted_past->val_bounds_.hor.pos(world_x) * casted_past->data_size_.hor - 0.5));
		x_idx_passed = std::min(x_idx_passed, casted_past->data_size_.hor - 1);
		if (casted_past->column_weight_vec_[x_idx_passed] == 0) continue;


		// Коэффициент для приведения плотностей к стандартной плотности
		double norm_koeff = double(trans_column_size) / casted_past->column_weight_vec_[x_idx_passed];

		size_t sum = 0;
		for (size_t base_y_idx = 0; base_y_idx < data_size_.vert; ++base_y_idx) {
			double world_y = val_bounds_.vert.lerp((base_y_idx + 0.5) / data_size_.vert);
			if (!casted_past->val_bounds_.vert.has_inside(world_y)) continue;

			size_t passed_y_idx = static_cast<size_t>(std::round(casted_past->val_bounds_.vert.pos(world_y) * casted_past->data_size_.vert - 0.5));
			passed_y_idx = std::min(passed_y_idx, casted_past->data_size_.vert - 1);

			auto cur_val = casted_past->data_[passed_y_idx * casted_past->data_size_.hor + x_idx_passed];
			cur_val = int64_t(std::ceil(cur_val * norm_koeff));
			new_max_trans_density = std::max<int64_t>(new_max_trans_density, cur_val);

			data_[base_y_idx * data_size_.hor + x_idx_base] = cur_val;
			sum += cur_val;
		}
		column_weight_vec_[x_idx_base] = sum;
	}

	// Если мы что-то скопировали – ускоряем затухание (переходный режим)
	if (new_max_trans_density > 0) {
		trans_decrease_counter_ = std::max(trans_decrease_counter_, new_max_trans_density);
	}

	is_data_updated_ = true;
	if ((hor_ratio <= 1.0) || (last_average_density_ == 0.0f)) {
		last_average_density_ = passed_data->last_average_density_;
	}
}

// ------------------------------------------------------------------
// UpdateQimage – отрисовка кадра с предварительным затуханием
// ------------------------------------------------------------------
void TileDPX::UpdateQimage(dynamic_qimage& dyn_qimage, const Limits<double>& power_bounds)
{
	// 1. Применяем затухание к данным перед отрисовкой
	ApplyDecay();

	// 2. Пересчитываем максимум и среднюю плотность (для цвета)
	UpdateDensityPivot();

	const int grid_height = data_size_.vert;
	const int grid_width = data_size_.hor;
	argb_t* rgb_iter = dyn_qimage.data.data();

	for (int vert_number = 0; vert_number < grid_height; ++vert_number)
	{
		const size_t* column_weight_iter = &column_weight_vec_[0];
		const auto matrix_idx_y = grid_height - vert_number - 1; // инверсия Y
		const int64_t* dpx_iter = &data_[matrix_idx_y * grid_width];

		for (int hor_number = 0; hor_number < grid_width; ++hor_number)
		{
			const double column_weight = *(column_weight_iter++);
			const double density = column_weight ? (*dpx_iter / column_weight) : 0.0;
			++dpx_iter;

			*rgb_iter++ = GetNormColor(density);
		}
	}
}

// ------------------------------------------------------------------
// Reset – полная очистка
// ------------------------------------------------------------------
void TileDPX::Reset()
{
	data_.clear();
	data_.resize(data_size_.hor * data_size_.vert, 0);
	column_weight_vec_.resize(data_size_.hor, 0);
	ippsZero_8u((Ipp8u*)column_weight_vec_.data(), column_weight_vec_.size() * sizeof(column_weight_vec_[0]));

	trans_decrease_counter_ = 0;
	last_average_density_ = 0.0;
}

void TileDPX::UpdateDecayInfo()
{
	//Здесь надо учитывать frame_time - если очень большое, то давать накопить хоть немного
	const double update_time_sec = update_elapsed_timer_.restart() / 1000.;
	const double elapsed_samples_per_column = std::max<int64_t>(1, periodical_block_counter_) * std::max<double>(1.0, last_samples_per_pixel_);

	const double min_time_for_good_image_sec = 100 * (update_time_sec / elapsed_samples_per_column);
	//Определяем время затухания при зумминге
	{
		double merged_data_decay_time = std::max<double>(min_time_for_good_image_sec, 1.);
		trans_decay_rate_ = std::exp(-2.0 * update_time_sec / merged_data_decay_time);
	}

	if (!need_decay_) {
		base_decay_rate_ = 1.0;
		periodical_block_counter_ = 0;
		return;
	}
	periodical_block_counter_ = 0;
	auto time_hold_sec = need_decay_ ? std::max<double>(min_time_for_good_image_sec, 1.) : 1;
	base_decay_rate_ = std::max(0.01, std::exp(-update_time_sec / time_hold_sec));
}
// ------------------------------------------------------------------
// Вспомогательные методы рисования (без изменений, но с уточнениями)
// ------------------------------------------------------------------
void TileDPX::DrawOnlyPoints(const std::vector<float>& data, const Limits<double>& src_freq)
{
	const size_t width = static_cast<size_t>(data_size_.hor);
	const size_t height = static_cast<size_t>(data_size_.vert);
	const auto& xb = val_bounds_.hor;
	const auto& yb = val_bounds_.vert;
	const double freq_step = src_freq.delta() / static_cast<double>(data.size() - 1);

	for (size_t i = 0; i < data.size(); ++i)
	{
		const double x = src_freq.low + i * freq_step;
		const double y = data[i];
		if (!xb.has_inside(x) || !yb.has_inside(y)) continue;

		size_t xi = static_cast<size_t>(xb.pos(x) * width);
		size_t yi = static_cast<size_t>(yb.pos(y) * height);
		if (xi == width)  --xi;
		if (yi == height) --yi;
		data_[yi * width + xi] += 100;
		column_weight_vec_[xi] += 100;
	}
}

void TileDPX::DrawInterpolated(const std::vector<float>& values, const Limits<double>& x_range)
{
	if (values.size() < 2) return;

	const auto& xb = val_bounds_.hor;
	const auto& yb = val_bounds_.vert;
	const int64_t width = data_size_.hor, height = data_size_.vert;
	const double step = x_range.delta() / (values.size() - 1);

	for (size_t i = 0; i + 1 < values.size(); ++i) {
		const double x0 = x_range.low + i * step;
		const double x1 = x0 + step;
		if (!xb.has_inside(x0) && !xb.has_inside(x1)) continue;

		SlopeInterpolator interp(values[i], values[i + 1]);

		double p0 = xb.pos(x0) * width;
		double p1 = xb.pos(x1) * width;

		int64_t x_begin = static_cast<int64_t>(std::floor(p0));
		x_begin = std::max(x_begin, 0i64);
		int64_t x_end = static_cast<int64_t>(std::ceil(p1));
		if (x_begin >= width) x_begin = width - 1;
		if (x_end > width)    x_end = width;
		if (x_end == 0 || x_begin >= width) continue;

		for (size_t px = x_begin; px < x_end; ++px) {
			const double x_world = xb.lerp((px + 0.5) / width);
			double t = (x_world - x0) / step;
			t = std::clamp(t, 0.0, 1.0);

			const double y_interp = interp.Interpolate(t);
			if (!yb.has_inside(y_interp)) continue;

			size_t py = static_cast<size_t>(std::round(yb.pos(y_interp) * height));
			if (py >= height) py = height - 1;

			data_[py * width + px] += 100;
			column_weight_vec_[px] += 100;
		}
	}
}

// ------------------------------------------------------------------
// GetNormColor – без изменений
// ------------------------------------------------------------------
argb_t TileDPX::GetNormColor(double relative_density) const
{
	const double normalized_density = qBound(0.0, relative_density / max_density_, 1.0);
	return LUT_HSV_Instance::DensityToRGB(normalized_density);
}

void TileDPX::ApplyDecay()
{
	UpdateDecayInfo();
	const int width = data_size_.hor;
	const int height = data_size_.vert;

	if (width == 0 || height == 0 || val_bounds_.hor.delta() <= 0)
		return;

	double decay_factor = base_decay_rate_;

	if (trans_decrease_counter_ > 0) {
		const double counter = static_cast<double>(trans_decrease_counter_);

		// До 100 transition затухает пропорционально мягко.
		// От 100 и выше используется максимальное transition decay.
		const double factor = std::min(1.0, counter / 2000.0);

		const double trans_factor = trans_decay_rate_;
			//1.0 + factor * (trans_decay_rate_ - 1.0);

		decay_factor *= trans_factor;
	}

	decay_factor = std::clamp(decay_factor, 0.001, 1.0);

	// Один и тот же итоговый коэффициент применяется ко всему состоянию.
	for (auto& val : data_)
		val = static_cast<int64_t>(val * decay_factor);

	for (auto& w : column_weight_vec_)
		w = static_cast<size_t>(w * decay_factor);

	trans_decrease_counter_ = static_cast<int64_t>(
		trans_decrease_counter_ * decay_factor
		);

	decay_factor_ = decay_factor;
	is_data_updated_ = true;
}
// ------------------------------------------------------------------
// UpdateDensityPivot – пересчёт максимума и средней плотности
// ------------------------------------------------------------------
void TileDPX::UpdateDensityPivot()
{
	// Здесь можно вычислить реальный максимум по данным, но для простоты оставим эмпирику
	// или вычислим на основе текущего состояния.
	// Важно, чтобы max_density_ и last_average_density_ отражали текущее состояние.
	// Предлагается простой способ: 
	//   max_density_ = 3.0 / data_size_.vert  (как в оригинале) 
	//   last_average_density_ – можно вычислить как среднее по всем ненулевым колонкам,
	//   но для упрощения оставим константу.
	last_average_density_ = 0.05;
	max_density_ = 1.0 / data_size_.vert * 3.0;
}