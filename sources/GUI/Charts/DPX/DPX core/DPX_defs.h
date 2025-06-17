#pragma once
#ifndef DPX_DEFS
#define DPX_DEFS

#include <stdint.h>
#include <vector>
#include <atomic>
#include <tbb/spin_mutex.h>
#include "aqua_defines.h"
#include "GUI/gui_defs.h"
using namespace fluctus;
using namespace aqua_gui;
namespace dpx_core
{
// Структура для хранения данных DPX
struct dpx_data 
{
    //Итоговый массив плотностей имеет такие шкалы:
    //Слева направо = частота, а снизу вверх = мощность (границы = val_bounds)
    typedef int64_t DataType;
    
    // Доступ к строкам данных
    DataType* operator[](size_t row) { return &data[row * size.horizontal]; }
    const DataType* operator[](size_t row) const { return &data[row * size.horizontal]; }

    WH_Bounds<double>       val_bounds; // Диапазон значений 
    WH_Info<size_t>         size;
    std::vector<DataType>   column_weight;   // Содержит суммарной вес каждой колонки. (size = width)
    std::vector<DataType>   data;            // (Плотность) Хранилище данных (size = высота * ширина) (values can not be negative)
    // Индикатор необходимости перерисовки
    std::atomic_bool        need_redraw = {false};
    tbb::spin_mutex         redraw_mutex;

};


}



#endif // DPX_DEFS