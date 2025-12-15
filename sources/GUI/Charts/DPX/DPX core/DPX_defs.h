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
    //Слева направо = частота, а сверху вниз - мощность (границы = val_bounds) (Т.е. КАРТИНКА вверх ногами!)
    typedef int64_t DataType;
    
    // Доступ к строкам данных
    DataType* operator[](size_t row) { return &data[row * size.horizontal]; }
    const DataType* operator[](size_t row) const { return &data[row * size.horizontal]; }

    HorVerLim<double>       val_bounds; // Диапазон значений 
    HV_Info<size_t>         size;
    std::vector<DataType>   column_weight;   // Содержит суммарной вес каждой колонки. (size = width)
    std::vector<DataType>   data;            // (Плотность) Хранилище данных (size = высота * ширина) (values can not be negative)
    // Индикатор необходимости перерисовки
    std::atomic_bool        need_redraw = {false};
    tbb::spin_mutex         redraw_mutex;
	std::atomic<int64_t>	n_fft_;

};


}



#endif // DPX_DEFS