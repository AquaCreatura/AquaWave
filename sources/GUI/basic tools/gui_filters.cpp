#include "gui_filters.h"
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

const int aqua_gui::GetAsimIntegerPlace(const int iteration_counter, const int width)
{
    // �������� ��������������� ��������� (�� 0 �� 1)
    double place = GetAsimDrawPlace(iteration_counter);

    // ����������� � ������ �������, �������� � ���������� ������ � ������� � �������� [0, width-1]
    int idx = static_cast<int>(place * width + 0.5);

    if (idx < 0)
        idx = 0;
    else if (idx >= width)
        idx = width - 1;

    return idx;
}
