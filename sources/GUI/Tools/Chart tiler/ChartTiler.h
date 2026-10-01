#pragma once
#include "Units/TileInterface.h"
#include "GUI/Tools/Chart drawers//QimageZoomer.h"
#include "GUI/Tools/gui_worker.h"
#include <future>
#include <qelapsedtimer.h>
using namespace aqua_gui;
//Works synchoniously
class ChartTiler { 

public:
	ChartTiler(const ChartScaleInfo& scale_info);
	void			SetData				(const draw_data& data);
	void			Reset				();
	const QPixmap&	GetRelevantPixmap	(bool is_optimized_mode = false);
	void			UpdateBounds		();
	TileInterface::uptr const & SpgGetTile() const;
	bool			SetLifeTime(const double life_time_sec);
protected:
	void			UpdateTileBase		(); 	//Init bounds of base image	
	void			UpdateTileView		();
	const QPixmap&	UpdateQPixmap		();
	bool			NeedUpdateTile		();
	void			UpdateFpsInfo		();
protected:
	int								 count_of_tiles_{ 3 };
	std::vector<TileInterface::uptr> tiles_;
	std::atomic<int>				 tile_id_;
	bool							 need_update_qimage_{false};
	const ChartScaleInfo&			scale_info_;
	dynamic_qimage					dyn_qim_; //Структура для работы с QImage в качестве обёртки
	QimageZoomer					zoomer_;
	const double					zoom_step_sqrt_ = 1.5;

	bool							is_spg_{ false };
	QElapsedTimer					image_update_timer_;
	tbb::spin_mutex					data_mutex_; //обновлять данные можем из разных потоков
	tbb::spin_mutex					bounds_mutex_; //обновлять границы можем из разных потоков...
	double							fps_default_ = 10;
	double							fps_optimization_ = 1;
	double							life_time_sec_;
	FpsEstimator					passed_data_fps_estimator_;
	FpsEstimator					show_frames_fps_estimator_;
};