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

}