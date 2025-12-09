#include "SelectionDrawer.h"
using namespace aqua_gui;

aqua_gui::SelectionDrawer::SelectionDrawer(const ChartScaleInfo & base_scale_info):
	scale_info_(base_scale_info)
{
}

bool aqua_gui::SelectionDrawer::DrawSelections(QPainter & painter)
{
	painter.save();
	auto &cur_chart_val = scale_info_.val_info_.cur_bounds;
	auto &base_chart_val = scale_info_.val_info_.min_max_bounds_;
	auto &chart_size_px = scale_info_.pix_info_.chart_size_px;

	QRect limit_rect = { 0,0, chart_size_px.horizontal, chart_size_px.vertical };
	painter.setClipRect(limit_rect);
	
	//painter
	{

		auto vert_sel = editable_selection_.bounds.vertical;
		auto hor_sel = editable_selection_.bounds.horizontal;

		if (vert_sel.high < vert_sel.low) std::swap(vert_sel.low, vert_sel.high);
		if (hor_sel.high < hor_sel.low) std::swap(hor_sel.low, hor_sel.high);

		Limits<int> hor_pix_limits = {
			std::lround((hor_sel.low - cur_chart_val.horizontal.low) / cur_chart_val.horizontal.delta() * chart_size_px.horizontal),
			std::lround((hor_sel.high - cur_chart_val.horizontal.low) / cur_chart_val.horizontal.delta() * chart_size_px.horizontal)
		};

		Limits<int> vert_pix_limits = {
			std::lround((cur_chart_val.vertical.high - vert_sel.high) / cur_chart_val.vertical.delta() * chart_size_px.vertical),
			std::lround((cur_chart_val.vertical.high - vert_sel.low) / cur_chart_val.vertical.delta() * chart_size_px.vertical)
		};

		Limits<int> hor_show_limits = {
			qBound(0, hor_pix_limits.low, chart_size_px.horizontal),
			qBound(0, hor_pix_limits.low, chart_size_px.horizontal)
		};

		Limits<int> vert_show_limits = {
			qBound(0, vert_pix_limits.low, chart_size_px.vertical),
			qBound(0, vert_pix_limits.low, chart_size_px.vertical)
		};



		QRect pixel_rect = { int(hor_pix_limits.low), int(vert_pix_limits.low), int(hor_pix_limits.delta()), int(vert_pix_limits.delta()) };

		// Заливка (наполовину прозрачная)
		QColor fillColor(0, 0, 255, 128); // синий, alpha 128 = 50% прозрачность
		painter.fillRect(pixel_rect, fillColor);

		// Рамка прямоугольника
		QPen pen(Qt::red);
		pen.setWidth(1); // толщина рамки
		painter.setPen(pen);
		painter.drawRect(pixel_rect);

	}

	painter.restore();
	return true;
}

void aqua_gui::SelectionDrawer::EditableEvent(const QPoint& mouse_location, const mouse_event_type event_type)
{
	auto &cur_chart_val = scale_info_.val_info_.cur_bounds;
	auto &chart_size_px = scale_info_.pix_info_.chart_size_px;

	const HV_Info<double> ratio_pos = {
		qBound(0, mouse_location.x(), chart_size_px.horizontal) / double(chart_size_px.horizontal),
		qBound(0, mouse_location.y(), chart_size_px.vertical) / double(chart_size_px.vertical)
	};
	const HV_Info<double> value_pos = {
		ratio_pos.horizontal * cur_chart_val.horizontal.delta() + cur_chart_val.horizontal.low,
		cur_chart_val.vertical.high - ratio_pos.vertical * cur_chart_val.vertical.delta()
	};
	switch (event_type)
	{
	case aqua_gui::SelectionDrawer::kMove:		//Изменияем селекшен (при необходимости)
		if (is_pressed_) {
			editable_selection_.bounds.horizontal.high = value_pos.horizontal;
			editable_selection_.bounds.vertical.high   = value_pos.vertical;
		}
		break;
	case aqua_gui::SelectionDrawer::kPressed:	//Начинаем селекшен
		is_pressed_ = true;
		editable_selection_.bounds.horizontal = { value_pos.horizontal,value_pos.horizontal };
		editable_selection_.bounds.vertical	  =	{ value_pos.vertical  ,value_pos.vertical };
		break;
	case aqua_gui::SelectionDrawer::kReleased:	//Заканчиваем
		is_pressed_ = false;
		break;
	default:
		break;
	}
}
