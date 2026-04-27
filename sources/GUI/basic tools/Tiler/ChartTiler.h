#pragma once
#include "TileInterface.h"
#include "GUI/Drawers/QimageZoomer.h"
#include <future>
using namespace aqua_gui;
//Works synchoniously
class ChartTiler { 

public:
	ChartTiler(const ChartScaleInfo& scale_info);
	void			SetData				(const draw_data& data);
	void			Reset				();
	const QPixmap&	GetRelevantPixmap	();
protected:
	void			UpdateBaseBounds	(); 	//Init bounds of base image	
	void			UpdateViewBounds	();
	void			UpdateDynamicQimage ();
protected:
	std::unique_ptr<TileInterface>	cur_tile_;
	std::unique_ptr<TileInterface>	buff_tile_;
	std::unique_ptr<TileInterface>	base_tile_;
	bool					need_use_base_tile_{ false };
	const ChartScaleInfo&	scale_info_;
	dynamic_qimage			dyn_qim_; //Структура для работы с QImage в качестве обёртки
	QimageZoomer			zoomer_;
	const double			zoom_step_sqrt_ = 1.5;

};