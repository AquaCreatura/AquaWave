#pragma once
#include <qstring.h>
#include <qgraphicsview.h>
#include <qgraphicsitem.h>
#include <qpainter.h>
#include "Tools\parse_tools.h"
#include "GUI/gui_defs.h"
namespace aqua_gui
{
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
	bool DrawSelections	(QPainter& painter);
	void EditableEvent	(const QPoint& mouse_location, const mouse_event_type event_type);
protected:
	struct sel_info {
		WH_Bounds<double> bounds;

	};
private:
	const ChartScaleInfo&  scale_info_;

	sel_info			   editable_selection_;
	bool				   is_pressed_{ false };
};

}