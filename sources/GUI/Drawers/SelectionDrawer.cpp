#include "SelectionDrawer.h"
using namespace aqua_gui;

//============================ SelectionHolder ================================
selection_info aqua_gui::SelectionHolder::GetCurrentSelection()
{
	return cur_sel_;
}

void aqua_gui::SelectionHolder::SetCurrentSelection(selection_info sel_info)
{
	cur_sel_ = sel_info;
}



//======================================== SelectionDrawer ====================================


aqua_gui::SelectionDrawer::SelectionDrawer(const ChartScaleInfo & base_scale_info):
	scale_info_(base_scale_info)
{
	sel_holder_ = std::make_shared<SelectionHolder>();
}

void aqua_gui::SelectionDrawer::SetSelectionHolder(std::shared_ptr<SelectionHolder> holder)
{
	if(holder)
		sel_holder_ = holder;
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
		auto cur_hv = GetHorVert(sel_holder_->GetCurrentSelection());

		auto vert_sel = cur_hv.vertical;
		auto hor_sel = cur_hv.horizontal;
	
		if (vert_sel.high < vert_sel.low) std::swap(vert_sel.low, vert_sel.high);
		if (hor_sel.high < hor_sel.low) std::swap(hor_sel.low, hor_sel.high);
	
		
		if ((vert_sel.high < cur_chart_val.vertical.low || vert_sel.low > cur_chart_val.vertical.high) &&
			(hor_sel.high < cur_chart_val.horizontal.low || hor_sel.low > cur_chart_val.horizontal.high))
			return true;
	
		HorVerLim<int> user_rect = {
			{ std::lround((hor_sel.low - cur_chart_val.horizontal.low) / cur_chart_val.horizontal.delta() * chart_size_px.horizontal),
			std::lround((hor_sel.high - cur_chart_val.horizontal.low) / cur_chart_val.horizontal.delta() * chart_size_px.horizontal) },
			std::lround((cur_chart_val.vertical.high - vert_sel.high) / cur_chart_val.vertical.delta() * chart_size_px.vertical),
			std::lround((cur_chart_val.vertical.high - vert_sel.low) / cur_chart_val.vertical.delta() * chart_size_px.vertical)
		};
		bool is_hor_valid = user_rect.horizontal.delta() >= 1;
		bool is_vert_valid = user_rect.vertical.delta() >= 1;
		if (!is_hor_valid && !is_vert_valid) return true;
	
		const QRect pixel_rect = { int(user_rect.horizontal.low), int(user_rect.vertical.low), 
										int(user_rect.horizontal.delta() + 1), int(user_rect.vertical.delta() + 1) };
		{
			QPen sel_pen;
			sel_pen.setColor(QColor(80, 80, 80, 255));
			
			QColor fillColor(0, 255, 255, 60); // синий, alpha 128 = 50% прозрачность
			sel_pen.setWidth(1);
			painter.setPen(sel_pen);
	
			if (is_hor_valid) {
				painter.drawLine(pixel_rect.left(), 0, pixel_rect.left(), chart_size_px.vertical); //Вертикальные линии
				painter.drawLine(pixel_rect.right() + 1, 0, pixel_rect.right() + 1, chart_size_px.vertical);
	
				QRect fillrect(QPoint(pixel_rect.left(), 0), QPoint({ pixel_rect.right(), chart_size_px.vertical }));
				painter.fillRect(fillrect, fillColor);
			}
			
	
			if (is_vert_valid) {
				painter.drawLine(0, pixel_rect.top(), chart_size_px.horizontal, pixel_rect.top()); //Горизонтальные линии
				painter.drawLine(0, pixel_rect.bottom() + 1 , chart_size_px.horizontal, pixel_rect.bottom() + 1);
	
				QRect fillrect(QPoint(0, pixel_rect.top()), QPoint({ chart_size_px.horizontal, pixel_rect.bottom()}));
				painter.fillRect(fillrect, fillColor);
			}
	
		}
	
		//Отрисовка самого селекшена
		if(is_hor_valid && is_vert_valid)
		{
			// Заливка (наполовину прозрачная)
			QColor fillColor(0, 0, 255, 65); // синий, alpha 128 = 50% прозрачность
			//painter.fillRect(pixel_rect, fillColor);
	
			// Рамка прямоугольника
			QPen pen;
			pen.setColor(QColor(255, 255, 255, 255));
			pen.setWidth(1); // толщина рамки
			painter.setPen(pen);
			painter.drawRect(pixel_rect);
		}
	
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
	case aqua_gui::SelectionDrawer::kMove:		
		if (!is_pressed_) return; //Выходим при пустой отрисовке.
		
		cur_hv_.horizontal.high = value_pos.horizontal;
		cur_hv_.vertical.high   = value_pos.vertical;
		
		break;
	case aqua_gui::SelectionDrawer::kPressed:	//Начинаем
		is_pressed_ = true;
		cur_hv_.horizontal = { value_pos.horizontal,value_pos.horizontal };
		cur_hv_.vertical	  =	{ value_pos.vertical  ,value_pos.vertical };
		break;
	case aqua_gui::SelectionDrawer::kReleased:	//Заканчиваем
		is_pressed_ = false;
		break;
	default:
		break;
	}
	ChangeCurSelection();
}

void aqua_gui::SelectionDrawer::ChangeCurSelection()
{
	selection_info sel_info;
	if (scale_info_.val_info_.domain_type == ChartDomainType::kTimeFrequency) {
		sel_info.freq_bounds = cur_hv_.vertical;
		sel_info.time_bounds = cur_hv_.horizontal / scale_info_.val_info_.min_max_bounds_.horizontal.delta();
	}
	else
	{
		sel_info.freq_bounds  = cur_hv_.horizontal;
		sel_info.time_bounds  = {0., 1.}; 
		sel_info.power_bounds = cur_hv_.vertical;
	}
	sel_holder_->SetCurrentSelection(sel_info);
}

HorVerLim<double> aqua_gui::SelectionDrawer::GetHorVert(const selection_info & sel_info)
{
	HorVerLim<double> hor_ver;
	if (scale_info_.val_info_.domain_type == ChartDomainType::kTimeFrequency) {
		hor_ver.horizontal = sel_info.time_bounds * scale_info_.val_info_.min_max_bounds_.horizontal.delta();
		hor_ver.vertical = sel_info.freq_bounds;
	}
	else
	{
		hor_ver.horizontal = sel_info.freq_bounds;
		hor_ver.vertical = sel_info.power_bounds;
	}
	return hor_ver;
}

