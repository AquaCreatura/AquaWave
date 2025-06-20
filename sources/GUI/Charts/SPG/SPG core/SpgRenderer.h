#pragma once
#include "spg_defs.h"
#include "qpixmap.h"
#include <qimage.h>
#include "GUI/Drawers/QimageZoomer.h"
#include "GUI/basic tools/gui_filters.h"
#include "GUI/basic tools/gui_conversions.h"

using  namespace fluctus;
using  namespace aqua_gui;

namespace spg_core
{
class SpgRenderer
{
public:


    //Bнициализаци¤ палитры
    SpgRenderer(spg_data &init_val);
    //
    QPixmap& GetRelevantPixmap(const ChartScaleInfo& scale_info);


protected:
    /*
        Update Qimage, it must has same size, as dpx data, but in RGB
    */
    bool    UpdateSpgRgbData();


    // ѕреобразует относительную плотность в цвет из палитры
    const argb_t* GetNormalizedColor( double relative_density) const;

private:
    spg_data              &sgp_;                  // Reference to the underlying data structure
    size_t                data_memory_ = 1'024;   // Cколько последних значений учитываем при отрисовке

    dynamic_qimage        dpx_rgb_;      // Структура для работы с QImage в качестве обёркти

    QPixmap               cached_pixmap_;
    QimageZoomer          zoomer_;

};   

};