#include "gui_helper.h"
#include <math.h>


const double aqua_gui::GetAsimDrawPlace(const int iteration_counter)
{
        // 1) Определяем уровень (слой в бинарном дереве)
    const int power = static_cast<int>(std::log2(iteration_counter + 1));
    const int count = 1 << power;             // число элементов на этом уровне
    const int min_elem = count - 1;           // индекс первого элемента этого уровня

    // 2) Индекс внутри уровня от 0 до count-1
    const int j = iteration_counter - min_elem;

    // 3) Вычисляем "последовательный" индекс с чередованием краевых:
    //    j=0→0 (левый), j=1→count-1 (правый), j=2→1, j=3→count-2 и т.д.
    int seq;
    if ((j & 1) == 0) {
        // чётный j: 0,2,4… → слева направо 0,1,2…
        seq = j / 2;
    } else {
        // нечётный j: 1,3,5… → справа налево count-1, count-2…
        seq = (count - 1) - (j / 2);
    }

    // 4) Приводим seq в центр ячейки [0,1]:
    const double step = 1.0 / count;
    return step * seq + step * 0.5;
}


std::vector<int> aqua_gui::GetAssimLocationsVec(const int width)
{
    // Вектор пар (позиция, итерация)
    std::vector<std::pair<double, int>> place_iteration_pairs;
    for (int i = 0; i < width; ++i) {
        double place = GetAsimDrawPlace_Ext(i);
        place_iteration_pairs.push_back({place, i});
    }

    // Сортировка по позициям, при равенстве — по номеру итерации
    std::sort(place_iteration_pairs.begin(), place_iteration_pairs.end());

    // Создание результирующего вектора
    std::vector<int> result(width);
    for (int k = 0; k < width; ++k) {
        int iteration = place_iteration_pairs[k].second;
        result[iteration] = k; // Индекс — итерация, значение — местоположение
    }

    return result;
}
