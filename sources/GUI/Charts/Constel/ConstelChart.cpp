#include "ConstelChart.h"
using namespace aqua_gui;
using namespace constel;
ChartConstel::ChartConstel(QWidget * parrent): 
	QWidget(parrent), bg_image_(scale_info_)
{
	bg_image_.InitImage(":/AquaWave/third_party/background/sym_sky.jpg");
	setMaximumWidth(257 * 5);
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


void ChartConstel::PushData(std::vector<Ipp32fc> & draw_data)
{
	core_.AddData(draw_data);
}

void ChartConstel::ClearData()
{
	core_.Emplace();
}

void ChartConstel::paintEvent(QPaintEvent * paint_event)
{
	QPainter new_frame_painter(this);
	bg_image_.DrawImage(new_frame_painter);
	{
		auto data_pixmap = core_.GetRelevantPixmap(scale_info_.pix_info_.chart_size_px.horizontal);
		new_frame_painter.drawPixmap(0,0, data_pixmap);
	}
	
	
	

}

void ChartConstel::resizeEvent(QResizeEvent * event)
{
	aqua_gui::HV_Info<int> cur_size = { this->width(), this->height() };
	auto        &pix_info = scale_info_.pix_info_;
	if (pix_info.widget_size_px == cur_size) return;
	pix_info.widget_size_px = cur_size;
	pix_info.chart_size_px = pix_info.widget_size_px - pix_info.margin_px;
	

	
	if (cur_size.vertical < cur_size.horizontal) {
		setMinimumHeight(cur_size.horizontal);
	}
	else if (cur_size.horizontal < cur_size.vertical) {
		setMinimumHeight(cur_size.horizontal);		
		const auto min_size = std::min(cur_size.horizontal, cur_size.vertical);
		//resize(min_size, min_size);
	}
	
		


}

bool ChartConstel::ShouldRedraw()
{
	return true;
}
