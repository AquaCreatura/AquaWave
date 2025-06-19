#pragma once
namespace aqua_gui
{


class SlopeInterpolator
{
public:
    //Initialising
    SlopeInterpolator(const double first_val, const double next_val): first_val_(first_val), slope_koeff_(next_val - first_val){};
    //ratio [0, 1) indicates location between first and second value
    constexpr double Interpolate(double ratio) const noexcept {return (first_val_ + (ratio * slope_koeff_));};
private:
    const double first_val_ = 0;
    const double slope_koeff_ = 0;
};

// ¬озвращает нормализованную позицию (0..1) центра элемента с индексом iteration_counter
const double GetAsimDrawPlace(const int iteration_counter);

// ¬озвращает индекс позиции дл€ отрисовки в диапазоне [0, width-1] на основе iteration_counter
const int GetAsimIntegerPlace(const int iteration_counter, const int width);

}