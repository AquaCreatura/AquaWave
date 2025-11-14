#pragma once

#include <stdint.h>
#include <vector>
#include <atomic>
#include <tbb/spin_mutex.h>
#include "GUI/gui_defs.h"
#include <tbb/spin_mutex.h>
using namespace fluctus;
using namespace aqua_gui;

namespace spg_core
{
    typedef float DataType;
	

	enum HolderStation {
		kNewData	= 1 << 0, //Требуются новые данные
		kPrecising  = 1 << 1, //Происходит уточнение данных		

		kReadyToUse = 1 << 2,
		kFullData   = 1 << 3,

		kRequestStation  = 1 << 10,
		kPreparingStation = 1 << 11,
	};

    struct spg_holder
    {
		
        //Итоговый массив плотностей имеет такие шкалы:
        //Слева направо = время, а сверху вниз - частота (границы = val_bounds) (Т.е. КАРТИНКА вверх ногами!)

        // Доступ к строкам данных
        DataType* operator[](size_t row) { return &data[row * size.horizontal]; }
        const DataType* operator[](size_t row) const { return &data[row * size.horizontal]; }

        WH_Bounds<double>					val_bounds;		//Диапазон значений 
        WH_Info<size_t>						size;			//Размерность массива
        std::vector<bool>					relevant_vec;	//Актуальность наших данных столбцов
        std::vector<DataType>				data;			//Наши данные
        std::atomic<bool>					need_redraw;
		mutable std::atomic<int>			state;			//Состояние нашего холдера
    };

    struct spg_data
    {
        mutable tbb::spin_mutex				rw_mutex_;		
        spg_holder							base_data;		//Данные целого полотна
		spg_holder							realtime_data;	//Отображаемые, зазумленные данные

        Limits<double>						power_bounds;
		std::atomic<int64_t>				n_fft_{1024};

    };


}