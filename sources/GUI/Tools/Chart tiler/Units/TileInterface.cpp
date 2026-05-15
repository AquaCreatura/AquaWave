#include "TileInterface.h"
#include "TileDPX.h"
#include "TileSPG.h"
void TileInterface::SetValBounds(HorVerLim<double> bounds)
{
	val_bounds_ = bounds;
	Reset();
}

void TileInterface::SetImageSize(const HV_Info<size_t> size)
{
	if (!is_rotated_)
		data_size_ = size;
	else {
		data_size_.hor	= size.vert;
		data_size_.vert = size.hor;
	}		
	Reset();
}

const HV_Info<size_t> TileInterface::GetImageSize() const
{
	if (!is_rotated_)
		return data_size_;
	else
		return { data_size_.vert, data_size_.hor };
}

std::unique_ptr<TileInterface> TileInterface::RecreateWithBounds(const HorVerLim<double>& bounds) const
{
	std::unique_ptr<TileInterface> new_tile;
	if (is_spg_)
		new_tile = (std::make_unique<TileSPG>());
	else
		new_tile = (std::make_unique<TileDPX>());
	new_tile->SetImageSize(GetImageSize());
	new_tile->SetValBounds(bounds);
	new_tile->UpdateFromTile(this);

	return new_tile;
}


HorVerLim<double> TileInterface::GetValBounds()
{
	return val_bounds_;
}



