#include "ConstelCore.h"
#include <tbb/parallel_for.h>
using namespace constel;
constel::ConstelCore::ConstelCore():
	renderer_(constel_)
{
}
void constel::ConstelCore::AddData(const std::vector<Ipp32fc> &passed_data)
{
	if (constel_.data.empty()) Emplace();
	CheckPassedMaximum(passed_data);
	StoreData(passed_data);
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
}

QPixmap & constel::ConstelCore::GetRelevantPixmap(const int chart_size_px)
{
	return renderer_.DrawData(chart_size_px);
}

void constel::ConstelCore::CheckPassedMaximum(const std::vector<Ipp32fc>& passed_data)
{
	auto casted_vec = (std::vector<Ipp32f>&)passed_data;
	float max_value = 0;
	ippsMaxAbs_32f(casted_vec.data(), casted_vec.size(), &max_value);
	if (constel_.max_power < max_value) SetNewMaximum(max_value * 1.1); //Добавляем минимальный зазор
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

	tbb::spin_mutex::scoped_lock lock(constel_.redraw_mutex);

	const auto& old_data = constel_.data;

	tbb::parallel_for(-A, A + 1, [&](int nx)
	{
		for (int ny = -A; ny <= A; ++ny)
		{
			float ox = nx * inv_scale;
			float oy = ny * inv_scale;

			int ix = int(ox + 0.5f);
			int iy = int(oy + 0.5f);

			if (abs(ix) > A || abs(iy) > A)
				continue;

			int new_index = (nx + A) * size + (ny + A);
			int old_index = (ix + A) * size + (iy + A);

			new_data[new_index] = old_data[old_index];
		}
	});

	constel_.data.swap(new_data);
	constel_.max_power = max_value;
}

void constel::ConstelCore::StoreData(const std::vector<Ipp32fc>& data)
{
	const float scale = constel_.side_amplitude / constel_.max_power;
	const int A = constel_.side_amplitude;
	const int size = constel_.side_size;

	auto& hist = constel_.data;

	tbb::spin_mutex::scoped_lock lock(constel_.redraw_mutex);

	for (const auto& sample : data)
	{
		int x = std::lround(sample.re * scale);
		int y = std::lround(sample.im * scale);

		if (std::abs(x) > A || std::abs(y) > A)
			continue;

		hist[(x + A) * size + (y + A)]++;
	}

	constel_.count_of_points += data.size();
}
