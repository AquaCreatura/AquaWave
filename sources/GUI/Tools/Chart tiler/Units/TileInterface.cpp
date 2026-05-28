#include "TileInterface.h"
#include "TileDPX.h"
#include "TileSPG.h"
void TileInterface::SetValBounds(HorVerLim<double> bounds)
{
	val_bounds_ = bounds;
	if (is_rotated_) {
		std::swap(val_bounds_.hor, val_bounds_.vert);
	}
	Reset();
}

void TileInterface::SetImageSize(const HV_Info<size_t> size)
{
	data_size_ = size;
	if (is_rotated_) {
		std::swap(data_size_.hor, data_size_.vert);
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
	if (!is_rotated_)
		return val_bounds_;
	else
		return { val_bounds_.vert, val_bounds_.hor };
}



