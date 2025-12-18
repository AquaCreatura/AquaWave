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
		auto val_hv = GetHorVert(sel_holder_->GetCurrentSelection());
		auto &vert_sel = val_hv.vertical;
		auto &hor_sel = val_hv.horizontal;

		if (vert_sel.high < vert_sel.low) std::swap(vert_sel.low, vert_sel.high);
		if (hor_sel.high < hor_sel.low) std::swap(hor_sel.low, hor_sel.high);
	
		HorVerLim<int> user_rect = {
			{ std::lround((hor_sel.low - cur_chart_val.horizontal.low) / cur_chart_val.horizontal.delta() * chart_size_px.horizontal),
			std::lround((hor_sel.high - cur_chart_val.horizontal.low) / cur_chart_val.horizontal.delta() * chart_size_px.horizontal) },
			std::lround((cur_chart_val.vertical.high - vert_sel.high) / cur_chart_val.vertical.delta() * chart_size_px.vertical),
			std::lround((cur_chart_val.vertical.high - vert_sel.low) / cur_chart_val.vertical.delta() * chart_size_px.vertical)
		};

		if (DrawRectangles(painter, user_rect)) {
			DrawSizes(painter, user_rect, val_hv);
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
		sel_info.time_bounds = cur_hv_.horizontal;
	}
	else
	{
		sel_info.freq_bounds  = cur_hv_.horizontal;
		sel_info.power_bounds = cur_hv_.vertical;
	}
	sel_holder_->SetCurrentSelection(sel_info);
}

HorVerLim<double> aqua_gui::SelectionDrawer::GetHorVert(const selection_info & sel_info)
{
	HorVerLim<double> hor_ver;
	if (scale_info_.val_info_.domain_type == ChartDomainType::kTimeFrequency) {
		hor_ver.horizontal = sel_info.time_bounds;
		hor_ver.vertical = sel_info.freq_bounds;
	}
	else
	{
		hor_ver.horizontal = sel_info.freq_bounds;
		hor_ver.vertical = sel_info.power_bounds;
	}
	return hor_ver;
}

bool aqua_gui::SelectionDrawer::DrawRectangles(QPainter & painter, const HorVerLim<int>& user_rect)
{
	auto &chart_size_px = scale_info_.pix_info_.chart_size_px;

	bool is_hor_valid = user_rect.horizontal.delta() >= 1;
	bool is_vert_valid = user_rect.vertical.delta() >= 1;
	if (!is_hor_valid && !is_vert_valid) return false;

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

			if (scale_info_.val_info_.domain_type != ChartDomainType::kTimeFrequency) {
				QRect fillrect(QPoint(pixel_rect.left(), 0), QPoint({ pixel_rect.right(), chart_size_px.vertical }));
				painter.fillRect(fillrect, fillColor);
			}
		}


		if (is_vert_valid) {
			painter.drawLine(0, pixel_rect.top(), chart_size_px.horizontal, pixel_rect.top()); //Горизонтальные линии
			painter.drawLine(0, pixel_rect.bottom() + 1, chart_size_px.horizontal, pixel_rect.bottom() + 1);

			if (scale_info_.val_info_.domain_type != ChartDomainType::kFreqDomain) {
				QRect fillrect(QPoint(0, pixel_rect.top()), QPoint({ chart_size_px.horizontal, pixel_rect.bottom() }));
				painter.fillRect(fillrect, fillColor);
			}
			
		}

	}

	//Отрисовка самого селекшена
	if (is_hor_valid && is_vert_valid)
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
	return true;
}
/*
user_rect - прямоугольник в пикселях.
hv_val	  - прямоугольник в значениях

*/
#define M_PI 3.14
bool aqua_gui::SelectionDrawer::DrawSizes(
	QPainter &painter,
	const HorVerLim<int>& user_rect,
	const HorVerLim<double>& hv_val)
{
	auto &chart = scale_info_.pix_info_.chart_size_px;
	const int chartW = chart.horizontal;
	const int chartH = chart.vertical;

	painter.save();
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setPen(QPen(Qt::white, 1));

	const int offset = 20;     // Отступ линии от прямоугольника
	const int arrowSize = 8;    // Размер наконечника
	const int textMargin = 5;   // Отступ текста от линии
	const int minSizeForInsideArrows = 40;

	int left = user_rect.horizontal.low;
	int right = user_rect.horizontal.high;
	int top = user_rect.vertical.low;
	int bottom = user_rect.vertical.high;

	// Вспомогательная функция для отрисовки только "головы" стрелки
	auto drawArrowHead = [&](QPointF tip, double angle) {
		QPointF p1 = tip + QPointF(std::cos(angle + M_PI / 6) * arrowSize,
			std::sin(angle + M_PI / 6) * arrowSize);
		QPointF p2 = tip + QPointF(std::cos(angle - M_PI / 6) * arrowSize,
			std::sin(angle - M_PI / 6) * arrowSize);
		painter.drawPolygon(QPolygonF() << tip << p1 << p2);
	};

	/* ===============================
	ЛОГИКА ПОЗИЦИОНИРОВАНИЯ И ОТРИСОВКИ
	=============================== */
	const int textGap = 1;      // Зазор между текстом и линией
	const int arrowExt = 15;    // Длина "хвостиков" для маленьких размеров
	const QColor bgColor(100, 100, 100, 180); // Цвет фона текста

											  // Определение Y (для ширины) - Приоритет СНИЗУ
	int y = (bottom + offset + 5 <= chartH) ? (bottom + offset) :
		(top - offset - 5 >= 0) ? (top - offset) : qBound(5, bottom + offset, chartH - 5);

	// Определение X (для высоты) - Приоритет СПРАВА
	int x = (right + offset + 15 <= chartW) ? (right + offset) :
		(left - offset - 15 >= 0) ? (left - offset) : qBound(5, right + offset, chartW - 5);

	auto drawDimension = [&](QPointF p1, QPointF p2, bool isVertical, QString text) {
		double len = isVertical ? std::abs(p2.y() - p1.y()) : std::abs(p2.x() - p1.x());
		bool small = len < 40;

		// 1. Линия и стрелки
		painter.setPen(QPen(Qt::white, 1));
		painter.drawLine(p1, p2);

		if (!small) {
			if (!isVertical) { drawArrowHead(p1, 0); drawArrowHead(p2, M_PI); }
			else { drawArrowHead(p1, M_PI / 2); drawArrowHead(p2, -M_PI / 2); }
		}
		else {
			if (!isVertical) {
				painter.drawLine(p1, p1 - QPointF(arrowExt, 0));
				painter.drawLine(p2, p2 + QPointF(arrowExt, 0));
				drawArrowHead(p1, M_PI); drawArrowHead(p2, 0);
			}
			else {
				painter.drawLine(p1, p1 - QPointF(0, arrowExt));
				painter.drawLine(p2, p2 + QPointF(0, arrowExt));
				drawArrowHead(p1, -M_PI / 2); drawArrowHead(p2, M_PI / 2);
			}
		}

		// 2. Текст с фоном (Над или Слева)
		QFontMetrics fm(painter.font());
		QRectF txtRect = fm.boundingRect(text);
		txtRect.adjust(-3, 0, 3, 0); // Небольшие поля для красоты фона
		QPointF center = (p1 + p2) / 2.0;

		painter.save();
		if (!isVertical) {
			// Смещение над линией: y - высота_текста - зазор
			QRectF bgRect(center.x() - txtRect.width() / 2.0, p1.y() - txtRect.height() - textGap,
				txtRect.width(), txtRect.height());

			painter.setPen(Qt::NoPen);
			painter.setBrush(bgColor);
			painter.drawRect(bgRect);

			painter.setPen(Qt::white);
			painter.drawText(bgRect, Qt::AlignCenter, text);
		}
		else {
			// Поворот и смещение слева от линии
			painter.translate(p1.x() - textGap, center.y());
			painter.rotate(-90);

			// В повернутой системе координат "слева" становится "сверху" (отрицательный Y)
			QRectF bgRect(-txtRect.width() / 2.0, -txtRect.height(), txtRect.width(), txtRect.height());

			painter.setPen(Qt::NoPen);
			painter.setBrush(bgColor);
			painter.drawRect(bgRect);

			painter.setPen(Qt::white);
			painter.drawText(bgRect, Qt::AlignCenter, text);
		}
		painter.restore();
	};

	// Вызовы отрисовки
	drawDimension(QPointF(left, y), QPointF(right, y), false,
		QString::number(hv_val.horizontal.high - hv_val.horizontal.low, 'f', 2));

	drawDimension(QPointF(x, top), QPointF(x, bottom), true,
		QString::number(hv_val.vertical.high - hv_val.vertical.low, 'f', 2));

	painter.restore();
	return true;
}


