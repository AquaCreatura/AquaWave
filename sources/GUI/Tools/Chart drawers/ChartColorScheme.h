#pragma once
#include <qcolor.h>

namespace aqua_gui
{

struct ChartColorScheme
{
	QColor axis_color;
	QColor text_color;
	QColor selection_border_color;
	QColor selection_fill_color;
	QColor grid_line_color;
	QColor label_bg_color;
	QColor mouse_label_bg_color;
	
	static ChartColorScheme darkMode()
	{
		ChartColorScheme scheme;
		scheme.axis_color = QColor(80, 80, 80, 255);
		scheme.text_color = QColor(255, 255, 255, 255);
		scheme.selection_border_color = QColor(255, 255, 255, 255);
		scheme.selection_fill_color = QColor(100, 100, 100, 130);
		scheme.grid_line_color = QColor(200, 200, 200, 255);
		scheme.label_bg_color = QColor(50, 50, 50, 180);
		scheme.mouse_label_bg_color = QColor(0, 0, 125, 255);
		return scheme;
	}
	
	static ChartColorScheme lightMode()
	{
		ChartColorScheme scheme;
		scheme.axis_color = QColor(200, 200, 200, 255);
		scheme.text_color = QColor(0, 0, 0, 255);
		scheme.selection_border_color = QColor(0, 0, 0, 255);
		scheme.selection_fill_color = QColor(200, 200, 200, 130);
		scheme.grid_line_color = QColor(100, 100, 100, 255);
		scheme.label_bg_color = QColor(200, 200, 200, 180);
		scheme.mouse_label_bg_color = QColor(200, 200, 255, 255);
		return scheme;
	}
};

struct AxisColorScheme
{
	QColor text_color;
	QColor grid_color;
	QColor frame_color;
	QColor margin_gradient_start;
	QColor margin_gradient_end;
	
	static AxisColorScheme darkMode()
	{
		AxisColorScheme scheme;
		scheme.text_color = QColor(180, 180, 180, 255);
		scheme.grid_color = QColor(0, 40, 25, 255);
		scheme.frame_color = scheme.grid_color;
		scheme.margin_gradient_start = QColor(35, 35, 35, 210);
		scheme.margin_gradient_end = QColor(15, 15, 15, 210);
		return scheme;
	}
	
	static AxisColorScheme lightMode()
	{
		AxisColorScheme scheme;
		scheme.text_color = QColor(0, 0, 0, 255);
		scheme.grid_color = QColor(220, 220, 220, 255);
		scheme.frame_color = QColor(180, 180, 180, 255);
		scheme.margin_gradient_start = QColor(240, 240, 240, 210);
		scheme.margin_gradient_end = QColor(220, 220, 220, 210);
		return scheme;
	}
};

}
