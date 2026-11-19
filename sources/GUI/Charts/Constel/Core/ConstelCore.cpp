#include "ConstelCore.h"

#include <tbb/parallel_for.h>

#include <algorithm>
#include <cmath>

using namespace constel;

namespace
{
	float PivotsMaxAmplitude(const std::vector<Ipp32fc>& pivots)
	{
		float max_amp = 0.f;
		for (const Ipp32fc& pivot : pivots)
			max_amp = std::max(max_amp, std::max(std::abs(pivot.re), std::abs(pivot.im)));
		return max_amp;
	}
	float GetMinEuclid(const std::vector<Ipp32fc>& pivots)
	{
		if (pivots.empty()) return 0.f;

		float min_sq = std::numeric_limits<float>::max();
		for (const Ipp32fc& pivot : pivots)
		{
			float sq = pivot.re * pivot.re + pivot.im * pivot.im;
			min_sq = std::min(min_sq, sq);
		}
		return std::sqrt(min_sq);
	}
}

constel::ConstelCore::ConstelCore():
	renderer_(constel_)
{
}
void constel::ConstelCore::AddData(const std::vector<Ipp32fc> &passed_data)
{
	//Emplace();
	if (constel_.data.empty()) Emplace();
	// Если заданы пивоты, масштаб берётся по ним и максимум по входящим
	// данным не пересчитывается (экономия на каждой порции данных).
	if (!constel_.has_pivots)
		CheckPassedMaximum(passed_data);
	StoreData(passed_data);
	is_updated_ = true;
}
constellation_data & constel::ConstelCore::GetConstelData()
{
	return constel_;
}

void constel::ConstelCore::Emplace(const int bins_amplitude)
{
	tbb::spin_mutex::scoped_lock lock(constel_.redraw_mutex);
	constel_.count_of_points = 0;
	constel_.side_amplitude = bins_amplitude;
	constel_.side_size = 2 * bins_amplitude + 1;
	constel_.max_power = 1.f;
	constel_.data.clear();
	constel_.data.resize(constel_.side_size * constel_.side_size, 0);

	// Данные только что очищены, поэтому масштаб по пивотам можно
	// выставить напрямую, без пересчёта содержимого.
	if (constel_.has_pivots) {
		const float amp = PivotsMaxAmplitude(constel_.pivots);
		
		if (amp > 0.f) {
			constel_.max_power = (amp + GetMinEuclid(constel_.pivots) * 0.5) * 1.1;
			constel_.averaged_amplitude = amp;
		}
	}
}

void constel::ConstelCore::SetPivots(const std::vector<Ipp32fc>& pivots)
{
	if (pivots.empty()) {
		ClearPivots();
		return;
	}

	const float amp = PivotsMaxAmplitude(pivots);
	if (amp <= 0.f)
		return;

	{
		tbb::spin_mutex::scoped_lock lock(constel_.redraw_mutex);
		constel_.pivots = pivots;
		constel_.has_pivots = true;
		constel_.pivots_revision++;
	}

	// Приводим текущий масштаб к пивотам (пересчёт уже накопленных данных).
	constel_.averaged_amplitude = amp;
	SetNewMaximum((amp + GetMinEuclid(constel_.pivots) * 0.5) * 1.1);
}

void constel::ConstelCore::ClearPivots()
{
	tbb::spin_mutex::scoped_lock lock(constel_.redraw_mutex);
	if (!constel_.has_pivots)
		return;
	constel_.pivots.clear();
	constel_.has_pivots = false;
	constel_.pivots_revision++;
}

QPixmap & constel::ConstelCore::GetRelevantPixmap(const int chart_size_px)
{
	if(is_updated_)
		ApplyDecay();
	is_updated_ = false;
	return renderer_.DrawData(chart_size_px);
}

void constel::ConstelCore::CheckPassedMaximum(const std::vector<Ipp32fc>& passed_data)
{
	auto casted_vec = (std::vector<Ipp32f>&)passed_data;
	float amplitude = 0;
	ippsMaxAbs_32f(casted_vec.data(), casted_vec.size(), &amplitude );
	
	const float alpha_up = 0.001f;
	const float alpha_down = 0.010f;
	const double scale_shift = 1.2;
	if (amplitude > constel_.averaged_amplitude)
		constel_.averaged_amplitude = constel_.averaged_amplitude * (1.0f - alpha_up) + amplitude * alpha_up;
	else
		constel_.averaged_amplitude = constel_.averaged_amplitude * (1.0f - alpha_down) + amplitude * alpha_down;

	if (constel_.averaged_amplitude > constel_.max_power || constel_.averaged_amplitude * scale_shift * scale_shift < constel_.max_power)
		SetNewMaximum(constel_.averaged_amplitude * scale_shift); //Добавляем минимальный зазор
}

void constel::ConstelCore::SetNewMaximum(const Ipp32f max_value)
{
	const float old_max = constel_.max_power;

	if (old_max == max_value)
		return;

	const float scale = old_max / max_value;
	const float inv_scale = 1.0f / scale;

	const int A = constel_.side_amplitude;
	const int size = constel_.side_size;

	std::vector<int> new_data(constel_.data.size(), 0);
	if (new_data.empty())
		return;
	tbb::spin_mutex::scoped_lock lock(constel_.redraw_mutex);
	constel_.count_of_points = 0;
	if (scale < 20 && scale > 0.3) {
		const auto& old_data = constel_.data;
#if 1
		tbb::parallel_for(-A, A + 1, [&](int ny)
			{
				for (int nx = -A; nx <= A; ++nx)
				{
					float ox = nx * inv_scale;
					float oy = ny * inv_scale;

					int ix = int(ox + 0.5f);
					int iy = int(oy + 0.5f);

					if (abs(ix) > A || abs(iy) > A)
						continue;

					int new_index = (ny + A) * size + (nx + A);
					int old_index = (iy + A) * size + (ix + A);
					const auto new_sample = old_data[old_index];
					constel_.count_of_points += new_sample;
					new_data[new_index] = new_sample;
				}
			});
#endif
	}
	constel_.data.swap(new_data);
	constel_.max_power = max_value;
}

void constel::ConstelCore::StoreData(const std::vector<Ipp32fc>& data)
{
	const float scale = constel_.side_amplitude / constel_.max_power;
	const int A = constel_.side_amplitude;
	const int size = constel_.side_size;

	tbb::spin_mutex::scoped_lock lock(constel_.redraw_mutex);
	auto& hist = constel_.data;
	int64_t success_points = 0;
	for (const auto& sample : data)
	{
		int x = std::lround(sample.re * scale);
		int y = std::lround(sample.im * scale);

		if (std::abs(x) > A || std::abs(y) > A)
			continue;
		success_points++;
		hist[(y + A) * size + (x + A)]++;
	}

	constel_.count_of_points += success_points;
}
void constel::ConstelCore::InitDecay(const double fps, const double time_hold_sec)
{
	if (fps <= 0.0 || time_hold_sec <= 0.0)
	{
		decay_coeff_ = 1.0;
		return;
	}

	decay_coeff_ = std::exp(-1.0 / (fps * time_hold_sec));
}

void constel::ConstelCore::ApplyDecay()
{
	if (decay_coeff_ == 1.0)
		return;

	tbb::spin_mutex::scoped_lock lock(constel_.redraw_mutex);

	int64_t count_of_points = 0;

	for (auto& value : constel_.data)
	{
		value = value * decay_coeff_;
		count_of_points += value;
	}

	constel_.count_of_points = count_of_points;
}
