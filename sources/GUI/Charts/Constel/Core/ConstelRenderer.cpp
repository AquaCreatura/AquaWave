#include "ConstelRenderer.h"
#include "GUI/basic tools/gui_conversions.h"
using namespace constel;
constel::ConstellRenderer::ConstellRenderer(constellation_data & constel_struct) : 
	constel_(constel_struct)
{
}

QPixmap & constel::ConstellRenderer::DrawData(const int side_size_px)
{
	if (cache_.last_dots_count != constel_.count_of_points || cache_.side_size_px != side_size_px)
		UpdatePixmap(side_size_px);
	return cache_.pixmap;
}

void constel::ConstellRenderer::UpdatePixmap(const int side_size_px)
{
	{
		tbb::spin_mutex::scoped_lock lock(constel_.redraw_mutex);
		if (constel_.data.empty()) return;
		const size_t clusters_side_size_px = constel_.side_size;
		if (dyn_qimage_.size.hor != clusters_side_size_px)
		{
			dyn_qimage_.data.resize(clusters_side_size_px * clusters_side_size_px);
			dyn_qimage_.qimage = QImage((uint8_t*)dyn_qimage_.data.data(), clusters_side_size_px, clusters_side_size_px, QImage::Format::Format_ARGB32);
			dyn_qimage_.size = { clusters_side_size_px, clusters_side_size_px };
		};
		ConstelToRGB();
	}

	auto scaled = dyn_qimage_.qimage.scaled(side_size_px, side_size_px, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	cache_.pixmap = QPixmap::fromImage(scaled);
}

argb_t constel::ConstellRenderer::GetRgbFromDensity(double density)
{
	return LUT_HSV_Instance::DensityToRGB(density);
}

void constel::ConstellRenderer::ConstelToRGB()
{
	const int side_size = constel_.side_size;
	const int total_pixels = side_size * side_size;

	const int* src = constel_.data.data();
	uint32_t* dst = dyn_qimage_.data.data();

	int max_density = 0;
	for (int i = 0; i < total_pixels; ++i) {
		if (src[i] > max_density)
			max_density = src[i];
	}

	const double inv_log_max = 1.0 / std::log1p(static_cast<double>(max_density));

	for (int i = 0; i < total_pixels; ++i) {
		const int density = src[i];

		if (density == 0) {
			dst[i] = 0;
			continue;
		}

		double normalized = double(density) / max_density * 4;  std::log1p(static_cast<double>(density)) * inv_log_max;
		dst[i] = GetRgbFromDensity(normalized);
	}
}