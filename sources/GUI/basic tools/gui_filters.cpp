#include "gui_filters.h"
#include <math.h>


const double aqua_gui::GetAsimDrawPlace(const int iteration_counter)
{
    const int power = log2(iteration_counter + 1);       // уровень (номер слоя) в дереве
    const int count_of_elements = 1 << power;             // количество элементов на этом уровне
    const double step_size = 1.0 / count_of_elements;     // размер шага (ячейки) на оси [0,1]
    const int min_elem = (1 << power) - 1;                 // индекс первого элемента уровня
    const double place = step_size * (iteration_counter - min_elem) + step_size / 2;  // центр ячейки текущего элемента
    return place;
}

const int aqua_gui::GetAsimIntegerPlace(const int iteration_counter, const int width)
{
    // Получаем нормализованное положение (от 0 до 1)
    double place = GetAsimDrawPlace(iteration_counter);

    // Преобразуем в индекс пикселя, округляя к ближайшему целому и зажимая в диапазон [0, width-1]
    int idx = static_cast<int>(place * width + 0.5);

    if (idx < 0)
        idx = 0;
    else if (idx >= width)
        idx = width - 1;

    return idx;
}
