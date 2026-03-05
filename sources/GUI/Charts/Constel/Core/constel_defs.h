#pragma once
#include <vector>
#include "aqua_defines.h"
#include "GUI/gui_defs.h"

using namespace fluctus;
using namespace aqua_gui;
namespace constel {


struct constellation_data
{
	typedef int DataType;
	int amplitude; //Амплитуда в одну из сторону 2A + 1 = width = height
	int side_size; //Длина одной стороны = 2 * amplitude + 1
	std::vector<DataType>   data;            // (Плотность) Хранилище данных (size = высота * ширина) (values can not be negative)
	// Доступ к строкам данных
	DataType* operator[](size_t row) {
		const int mid_row = amplitude + row;
		const int mid_column = amplitude;
		return &data[mid_row * side_size + mid_column];
	}
	int64_t count_of_points;
	tbb::spin_mutex redraw_mutex;

};

}