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
	const int clusters_side_size_px = constel_.side_size;
	if (dyn_qimage_.side_size_px != clusters_side_size_px)
	{
		dyn_qimage_.data.resize(clusters_side_size_px * clusters_side_size_px);
		dyn_qimage_.qimage = QImage((uint8_t*)dyn_qimage_.data.data(), clusters_side_size_px, clusters_side_size_px, QImage::Format::Format_ARGB32);
		dyn_qimage_.side_size_px = clusters_side_size_px;
	};
	ConstelToRGB();

	auto scaled = dyn_qimage_.qimage.scaled(side_size_px, side_size_px, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	cache_.pixmap = QPixmap::fromImage(scaled);
}

argb_t constel::ConstellRenderer::GetRgbFromDensity(double density)
{
	
	double normalized_density = density;
	// Validate palette size
	if ((normalized_density <= 0))
	{
		static const uint8_t default_color[4] = { 0, 0, 0, 0 }; // Default to black (little-endian)
		return (uint32_t)(default_color);
	}
	normalized_density = qBound(0.0, normalized_density * normalized_density, 1.0);
	argb_t* color_palette = LUT_HSV_Instance::get_table_ptr();
	// Calculate palette index and clamp within valid range
	int color_index = static_cast<int>(normalized_density * (hsv_table_size_c - 1));
	color_index = qBound(0, color_index, hsv_table_size_c - 1);

	// Return RGBA color from palette 
	return (uint32_t)(color_palette + color_index);
}

void constel::ConstellRenderer::ConstelToRGB()
{
	const int size = constel_.side_size;
	const int total = size * size;

	const int* src = constel_.data.data();
	uint32_t* dst = dyn_qimage_.data.data();

	int max_density = 0;

	const int* p = src;
	const int* end = src + total;

	while (p != end)
	{
		if (*p > max_density)
			max_density = *p;
		++p;
	}

	if (max_density == 0)
	{
		std::memset(dst, 0, total * sizeof(uint32_t));
		return;
	}

	const double inv_log_max = 1.0 / std::log1p((double)max_density);

	const int* s = src;
	uint32_t* d = dst;

	while (s != end)
	{
		int v = *s++;

		if (v == 0)
		{
			*d++ = 0;
			continue;
		}

		double norm = std::log1p((double)v) * inv_log_max;

		*d++ = GetRgbFromDensity(norm);
	}
}
