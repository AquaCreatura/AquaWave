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
	QColor selection_label_bg_color;
	QColor axis_label_bg_color;
	QColor mouse_label_bg_color;
	
	static ChartColorScheme darkMode()
	{
		ChartColorScheme scheme;
		scheme.axis_color = QColor(80, 80, 80, 255);
		scheme.text_color = QColor(255, 255, 255, 255);
		scheme.selection_border_color = QColor(255, 255, 255, 255);
		scheme.selection_fill_color = QColor(100, 100, 100, 130);
		scheme.grid_line_color = QColor(200, 200, 200, 255);
		scheme.selection_label_bg_color = QColor(50, 50, 50, 180);
		scheme.axis_label_bg_color = QColor(50, 50, 50, 255);
		scheme.mouse_label_bg_color = QColor(0, 0, 125, 255);
		return scheme;
	}
	
	static ChartColorScheme lightMode()
	{
		ChartColorScheme scheme;

		scheme.axis_color = QColor(36, 112, 176, 255);        // #2470B0


		scheme.text_color = QColor(31, 45, 58, 255);          // #1F2D3A

		scheme.selection_border_color = QColor(255, 159, 67, 255); // #FF9F43


		scheme.selection_fill_color = QColor(255, 159, 67, 100);   // #FF9F43 @ ~40%


		scheme.grid_line_color = QColor(183, 210, 234, 255);  // #B7D2EA


		scheme.selection_label_bg_color = QColor(255, 178, 102, 200); // #FFB266 @ ~78%

		scheme.axis_label_bg_color = QColor(234, 243, 251, 255);  // #EAF3FB

		scheme.mouse_label_bg_color = QColor(255, 227, 199, 255); // #FFE3C7

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
		scheme.grid_color = QColor(125, 125, 125, 125);
		scheme.frame_color = scheme.grid_color;
		scheme.margin_gradient_start = QColor(35, 35, 35, 210);
		scheme.margin_gradient_end = QColor(15, 15, 15, 210);
		return scheme;
	}
	
	static AxisColorScheme lightMode()
	{
		AxisColorScheme scheme;
		scheme.text_color = QColor(0, 0, 0, 255);
		scheme.grid_color = QColor(125, 125, 125, 125);
		scheme.frame_color = QColor(180, 180, 180, 255);
		scheme.margin_gradient_start = QColor(240, 240, 240, 210);
		scheme.margin_gradient_end = QColor(220, 220, 220, 210);
		return scheme;
	}
};

}
