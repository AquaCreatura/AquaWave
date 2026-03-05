#pragma once
#include "GUI/Charts/ChartInterface.h"

class ChartConstel
{
public:
	ChartConstel(QWidget* parrent = nullptr);
	~ChartConstel();
	void DrawData(QPainter& painter);
	void PushData(const draw_data& draw_data);
	void ClearData();
protected:
	bool ShouldRedraw();
protected:
	QPixmap             cached_pixmap_;

};

