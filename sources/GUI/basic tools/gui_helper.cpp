#include "gui_helper.h"
#include <math.h>


const double aqua_gui::GetAsimDrawPlace(const int iteration_counter)
{
    const int power = log2(iteration_counter + 1);       // ������� (����� ����) � ������
    const int count_of_elements = 1 << power;             // ���������� ��������� �� ���� ������
    const double step_size = 1.0 / count_of_elements;     // ������ ���� (������) �� ��� [0,1]
    const int min_elem = (1 << power) - 1;                 // ������ ������� �������� ������
    const double place = step_size * (iteration_counter - min_elem) + step_size / 2;  // ����� ������ �������� ��������
    return place;
}

std::vector<int> aqua_gui::GetAssimLocationsVec(const int width)
{
    // ������ ��� (�������, ��������)
    std::vector<std::pair<double, int>> place_iteration_pairs;
    for (int i = 0; i < width; ++i) {
        double place = GetAsimDrawPlace(i);
        place_iteration_pairs.push_back({place, i});
    }

    // ���������� �� ��������, ��� ��������� � �� ������ ��������
    std::sort(place_iteration_pairs.begin(), place_iteration_pairs.end());

    // �������� ��������������� �������
    std::vector<int> result(width);
    for (int k = 0; k < width; ++k) {
        int iteration = place_iteration_pairs[k].second;
        result[iteration] = k; // ������ � ��������, �������� � ��������������
    }

    return result;
}
