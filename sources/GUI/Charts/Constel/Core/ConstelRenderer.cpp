#include "ConstelRenderer.h"
using namespace constel;
constel::ConstellRenderer::ConstellRenderer(constellation_data & constel_struct) : 
	data_(constel_struct)
{
}

QPixmap & constel::ConstellRenderer::DrawData(const int side_size_px)
{
	if (cache_.last_dots_count != data_.count_of_points || cache_.side_size_px != side_size_px)
		UpdatePixmap(side_size_px);
	return cache_.pixmap;
}

void constel::ConstellRenderer::UpdatePixmap(const int side_size_px)
{
}
