#pragma once
#include <qstring.h>
#include <qgraphicsview.h>
#include <qgraphicsitem.h>
#include <qpainter.h>
#include "Tools\parse_tools.h"
#include "GUI/gui_defs.h"
namespace aqua_gui
{

//Class, which hold selections
class SelectionHolder {

public:
	selection_info GetCurrentSelection();
	void		   SetCurrentSelection(selection_info sel_info);
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
class SelectionDrawer 
{
	
public:
	enum mouse_event_type {
		kMove,
		kPressed,
		kReleased
	};
	SelectionDrawer		(const ChartScaleInfo& base_scale_info);
	void SetSelectionHolder(std::shared_ptr<SelectionHolder> holder);
	bool DrawSelections	(QPainter& painter);
	void EditableEvent	(const QPoint& mouse_location, const mouse_event_type event_type);
protected:
	void ChangeCurSelection();
	HorVerLim<double> GetHorVert(const selection_info& sel_info);
protected: //Draw functions
	bool DrawRectangles	(QPainter& painter, const HorVerLim<int>& user_rect);
	bool DrawSizes		(QPainter& painter, const HorVerLim<int>& user_rect, const HorVerLim<double> &hv_val);
private:
	const ChartScaleInfo&			 scale_info_;
	std::shared_ptr<SelectionHolder> sel_holder_;
	HorVerLim<double>				 cur_hv_;
	bool							 is_pressed_{ false };
};

}