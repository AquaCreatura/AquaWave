#include "TileInterface.h"
void TileInterface::SetValBounds(HorVerLim<double> bounds)
{
	unit_.val_bounds = bounds;
}

void TileInterface::Reset()
{
}

HorVerLim<double> TileInterface::GetValBounds()
{
	return unit_.val_bounds;
}

void TileInterface::SetData(const draw_data & data)
{
}

const TilerUnit & TileInterface::GetDataUnit()
{
	return unit_;
}

void TileInterface::UpdateDataFromDataUnit(const TilerUnit & passed_data)
{
}
