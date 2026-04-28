#include "TileInterface.h"
void TileInterface::SetValBounds(HorVerLim<double> bounds)
{
	val_bounds_ = bounds;
}

void TileInterface::SetImageSize(const HV_Info<size_t>& size)
{
	image_size_ = size;
}

const HV_Info<size_t>& TileInterface::GetImageSize()
{
	return image_size_;
}

void TileInterface::Reset()
{
}

HorVerLim<double> TileInterface::GetValBounds()
{
	return val_bounds_;
}

void TileInterface::SetData(const draw_data & data)
{
}

