#include "ConstelChart.h"
using namespace aqua_gui;
using namespace constel;
ChartConstel::ChartConstel(QWidget * parrent): 
	QWidget(parrent), bg_image_(scale_info_)
{
	bg_image_.InitImage(":/AquaWave/sources/GUI/External/background/sym_sky.jpg");
	setMaximumWidth(257 * 5);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	scale_info_.pix_info_.margin_px = { 0,0 };

	scale_info_.val_info_.min_max_bounds.hor = { 0,1 };
	scale_info_.val_info_.min_max_bounds.vert = { 0,1 };
	
	scale_info_.val_info_.view_bounds.hor = { 0,1 };
	scale_info_.val_info_.view_bounds.vert = { 0,1 };

	connect(&redraw_timer_, &QTimer::timeout, this, QOverload<>::of(&ChartConstel::update));
	{
		double fps = 20;
		redraw_timer_.start(1000 / fps);
		core_.InitDecay(fps, 2);
	}
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

	//Выставляем закруглённые края
	{
		new_frame_painter.setRenderHint(QPainter::Antialiasing, true);
		QPainterPath path;
		path.addRoundedRect(rect(), 10, 10);
		new_frame_painter.setClipPath(path);
	}

	bg_image_.DrawImage(new_frame_painter);

	{
		auto chart_size = scale_info_.pix_info_.chart_size_px;
		auto data_pixmap = core_.GetRelevantPixmap(std::min(chart_size.hor, chart_size.vert));
		new_frame_painter.drawPixmap(0,0, data_pixmap);
	}

}

void ChartConstel::resizeEvent(QResizeEvent * event)
{
	aqua_gui::HV_Info<int> cur_size = { this->width(), this->height() };
	auto        &pix_info = scale_info_.pix_info_;
	if (pix_info.widget_size_px == cur_size) return;
	auto min_size = std::min(cur_size.hor, cur_size.vert);

	pix_info.widget_size_px = { min_size , min_size };
	pix_info.chart_size_px = pix_info.widget_size_px - pix_info.margin_px;
	resize({ min_size , min_size });
}

bool ChartConstel::ShouldRedraw()
{
	return true;
}
