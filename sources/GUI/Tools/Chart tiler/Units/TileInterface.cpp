#include "TileInterface.h"
void TileInterface::SetValBounds(HorVerLim<double> bounds)
{
	val_bounds_ = bounds;
}

void TileInterface::SetImageSize(const HV_Info<size_t>& size)
{

	if (!is_rotated_)
		data_size_ = size;
	else {
		data_size_.hor	= size.vert;
		data_size_.vert = size.hor;
	}
		
}

const HV_Info<size_t> TileInterface::GetImageSize()
{
	if (!is_rotated_)
		return data_size_;
	else
		return { data_size_.vert, data_size_.hor };
}

void TileInterface::Reset()
{
}

HorVerLim<double> TileInterface::GetValBounds()
{
	return val_bounds_;
}



