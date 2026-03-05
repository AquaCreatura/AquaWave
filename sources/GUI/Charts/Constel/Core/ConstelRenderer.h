#pragma once
#include "constel_defs.h"
#include <qpixmap.h>
namespace constel {


class ConstellRenderer {
public:
	ConstellRenderer(constellation_data &constel_struct);
	QPixmap& DrawData(const int side_size_px); //отрисовка 
protected:
	void UpdatePixmap(const int side_size_px);
protected:
	struct cache_pixmap_structure {
		int		side_size_px;
		QPixmap pixmap;
		int64_t last_dots_count;
	};
	cache_pixmap_structure	cache_;
	constellation_data		&data_;
	dynamic_qimage          dpx_rgb_; // Структура для работы с QImage в качестве обёркти
};















}