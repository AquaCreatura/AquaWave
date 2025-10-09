#pragma once
#include "DPX_defs.h"
#include "qpixmap.h"
#include <qimage.h>
#include "GUI/Drawers/QimageZoomer.h"
#include "GUI/basic tools/gui_helper.h"
#include "GUI/basic tools/gui_conversions.h"
#include <qelapsedtimer.h>
using namespace aqua_gui;
namespace dpx_core
{


class DpxRenderer
{
public:


    //Bнициализаци¤ палитры
    DpxRenderer(dpx_data &init_val);
    //
    QPixmap& GetRelevantPixmap(const ChartScaleInfo& scale_info);

protected:
    /*
        Update Qimage, it must has same size, as dpx data, but in RGB
    */
    bool    UpdateDpxRgbData();


    // ѕреобразует относительную плотность в цвет из палитры
    const argb_t* GetNormalizedColor( double relative_density) const;

private:
    dpx_data    &dpx_;                  // Reference to the underlying data structure
    size_t      data_memory_ = 1'024;   // Cколько последних значений учитываем при отрисовке

    dynamic_qimage        dpx_rgb_;      // Структура для работы с QImage в качестве обёркти

    QPixmap               cached_pixmap_;
    QimageZoomer          zoomer_;
	QElapsedTimer		  data_update_timer_; 
	double				  last_average_density_	{1.};
	double				  last_max_density_		{1.};
};
}