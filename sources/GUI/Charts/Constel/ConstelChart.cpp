#include "ConstelChart.h"
using namespace aqua_gui;
using namespace constel;
ChartConstel::ChartConstel(QWidget * parrent): 
	QWidget(parrent), bg_image_(scale_info_)
{
	bg_image_.InitImage(":/AquaWave/third_party/background/black_mountain.jpg");
	setMaximumWidth(257);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	scale_info_.pix_info_.margin_px = { 0,0 };

	scale_info_.val_info_.min_max_bounds_.horizontal = { 0,1 };
	scale_info_.val_info_.min_max_bounds_.vertical = { 0,1 };
	
	scale_info_.val_info_.cur_bounds.horizontal = { 0,1 };
	scale_info_.val_info_.cur_bounds.vertical = { 0,1 };

}

ChartConstel::~ChartConstel()
{
}

void ChartConstel::DrawData(QPainter & painter)
{
}

void ChartConstel::PushData(const draw_data & draw_data)
{
}

void ChartConstel::ClearData()
{
}

void ChartConstel::paintEvent(QPaintEvent * paint_event)
{
	QPainter new_frame_painter(this);
	{
		bg_image_.DrawImage(new_frame_painter);
	}
}

void ChartConstel::resizeEvent(QResizeEvent * event)
{
	if (width() < height())
		setMinimumWidth(height());		
	aqua_gui::HV_Info<int> cur_size = { this->width(), this->height() };
	auto        &pix_info = scale_info_.pix_info_;
	if (pix_info.widget_size_px == cur_size) return;
	pix_info.widget_size_px = cur_size;
	pix_info.chart_size_px = pix_info.widget_size_px - pix_info.margin_px;
	
	


}

bool ChartConstel::ShouldRedraw()
{
	return true;
}
