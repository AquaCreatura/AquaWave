#pragma once

#include <stdint.h>
#include <vector>
#include <atomic>
#include <tbb/spin_mutex.h>
#include "GUI/gui_defs.h"
using namespace fluctus;
using namespace aqua_gui;

namespace spg_core
{
    typedef uint8_t DataType;

    struct spg_data
    {
        //Итоговый массив плотностей имеет такие шкалы:
        //Слева направо = время, а сверху вниз - частота (границы = val_bounds) (Т.е. КАРТИНКА вверх ногами!)

        // Доступ к строкам данных
        DataType* operator[](size_t row) { return &data[row * size.horizontal]; }
        const DataType* operator[](size_t row) const { return &data[row * size.horizontal]; }

        WH_Bounds<double>       val_bounds; // Диапазон значений 
        WH_Info<size_t>         size;
        std::vector<bool>       relevant_vec; //Актуальность наших данных столбцов
        std::vector<DataType>   data;         //Наши данные
    };

    struct spg_holder
    {
        spg_data base_data; //Наши данные 


     


    };


}