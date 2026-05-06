#pragma once
#include "Units/TileInterface.h"
#include "GUI/Tools/Chart drawers//QimageZoomer.h"
#include <future>
using namespace aqua_gui;
//Works synchoniously
class ChartTiler { 

public:
	ChartTiler(const ChartScaleInfo& scale_info);
	void			SetData				(const draw_data& data);
	void			Reset				();
	const QPixmap&	GetRelevantPixmap	();
	void			UpdateBounds		();
protected:
	void			UpdateTileBase		(); 	//Init bounds of base image	
	void			UpdateTileView		();
	void			UpdateImageFromTile ();
	bool			NeedUpdateImage		();
protected:
	int								 count_of_tiles_{ 3 };
	std::vector<TileInterface::uptr> tiles_;
	std::atomic<int>				 tile_id_;
	
	const ChartScaleInfo&			scale_info_;
	dynamic_qimage					dyn_qim_; //—труктура дл€ работы с QImage в качестве обЄртки
	QimageZoomer					zoomer_;
	const double					zoom_step_sqrt_ = 1.5;

	QElapsedTimer					image_update_timer_;
	tbb::spin_mutex					update_bounds_mutex_; //обновл€ть границы можем из разных потоков
};