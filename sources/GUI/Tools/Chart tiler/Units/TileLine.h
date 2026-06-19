#pragma once
#include "TileInterface.h"



class TileLine : public TileInterface
{
public:
	enum class LineMode {
		MaxHold = 1,
		MeanHold = 2,
		FullHold = 3
	};
public:
	TileLine(LineMode line_mode);

	virtual void SetData(const draw_data& data) override;
	virtual void UpdateFromTile(const TileInterface* passed_data) override;
	virtual void UpdateQimage(dynamic_qimage& dyn_qimage, const Limits<double>& power_bounds) override;
	void Reset() override;
};