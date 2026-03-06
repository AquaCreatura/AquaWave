#pragma once
#include <qwidget.h>
#include "GUI/gui_defs.h"
#include "GUI/Drawers/ImageBackGround.h"
using namespace aqua_gui;
namespace constel {

class ChartConstel : public QWidget
{
	Q_OBJECT
public:
	ChartConstel(QWidget* parrent = nullptr);
	~ChartConstel();
	void DrawData(QPainter& painter);
	void PushData(const draw_data& draw_data);
	void ClearData();
	bool hasHeightForWidth() const override { return true; }
	int heightForWidth(int w) const override { return w; }
public: //Override
	// Handle widget repaint requests
	virtual void paintEvent(QPaintEvent* paint_event) override;
	void resizeEvent(QResizeEvent* event);
protected:
	bool ShouldRedraw();
protected:
	QPixmap             cached_pixmap_;

	aqua_gui::ChartScaleInfo scale_info_; // Chart scaling information

	aqua_gui::ImageBG bg_image_; // Background image for chart
};


}