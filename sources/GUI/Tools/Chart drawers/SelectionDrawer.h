#pragma once
#include <qstring.h>
#include <qgraphicsview.h>
#include <qgraphicsitem.h>
#include <qpainter.h>
#include "Utilities\parse_tools.h"
#include "GUI/gui_defs.h"
namespace aqua_gui
{

//Class, which hold selections
class SelectionHolder {

public:
	selection_info GetSelection();
	void		   ClearSelection();
	void		   UpdateSelection(selection_info sel_info);
	HorVerLim<double> GetHorVert(const ChartScaleInfo& scale_info);
protected:
	selection_info	cur_sel_;


};
/*
Class, which works with selections
     _____________
	|			  |
	| 4.924MHz 	  |
	|   QPSK	  |
	|_____________|
*/
enum class mouse_event_type {
	kMove,
	kPressedLeft,
	kPressedRight,
	kReleasedLeft,
	kReleasedRight
};
class SelectionDrawer 
{
	
public:
	SelectionDrawer		(const ChartScaleInfo& base_scale_info);
	void SetHolder(std::shared_ptr<SelectionHolder> holder);
	std::shared_ptr<SelectionHolder> GetHolder();
	bool DrawSelections	(QPainter& painter);
	void EditableEvent	(const QPoint& mouse_location, const mouse_event_type event_type);
protected:
	void ChangeCurSelection();
protected: //Draw functions
	bool DrawRectangles	(QPainter& painter, const HorVerLim<int>& user_rect);
	bool DrawSizes		(QPainter& painter, const HorVerLim<int>& user_rect, const HorVerLim<double> &hv_val);
	bool DrawMarks		(QPainter& painter, const HorVerLim<int>& user_rect, const HorVerLim<double> &hv_val);
private:
	const ChartScaleInfo&			 scale_info_;
	std::shared_ptr<SelectionHolder> sel_holder_;
	HorVerLim<double>				 cur_hv_;
	bool							 is_pressed_{ false };
};



class MouseDrawer
{
public:
	MouseDrawer(ChartScaleInfo& base_scale_info);

	void MouseEvent(const QPoint& mouse_location, mouse_event_type event_type);
	void SetWidgetInsideState(bool is_enter);
	bool Draw(QPainter& painter);

	// Возвращает форму курсора в зависимости от состояния
	Qt::CursorShape GetCursor() const;

protected:
	QPoint pos_;
	bool is_inside_widget_{ false };
	ChartScaleInfo& scale_info_;

	bool is_panning_ = false;
	QPoint pan_start_pos_;
	HV_Info<double, double> pan_world_pos_;   // точка в мировых координатах, за которую схватились
};
}