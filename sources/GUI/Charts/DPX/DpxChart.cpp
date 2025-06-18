#include "DpxChart.h"

ChartDPX::ChartDPX(QWidget * parrent):
    ChartInterface(parrent)
{
    SetHorizontalMinMaxBounds(50'000, 200'000);
    SetHorizontalSuffix("counts");

    SetVerticalMinMaxBounds(10, 80, true);
    SetVerticalSuffix("power");
}

ChartDPX::~ChartDPX()
{

}

void ChartDPX::DrawData(QPainter & passed_painter)
{
    if(ShouldRedraw()) 
    {
        cached_pixmap_ = dpx_painter_.GetRelevantPixmap(scale_info_);
    }
    if(cached_pixmap_.isNull()) 
        return;
    passed_painter.drawPixmap(0, 0, cached_pixmap_);
}

void ChartDPX::PushData(std::vector<float>& data, const Limits<double>& data_bounds)
{
    power_man_.UpdateBounds(data, scale_info_.val_info_.min_max_bounds_.horizontal /*data_bounds*/);
    dpx_painter_.AccumulateNewData(data, scale_info_.val_info_.min_max_bounds_.horizontal);
}

void ChartDPX::SetVerticalMinMaxBounds(const double min_val, const double end_val, const bool is_adaptive)
{
    dpx_painter_.SetPowerBounds({min_val, end_val});
    ChartInterface::SetVerticalMinMaxBounds(min_val, end_val, is_adaptive);
}


bool ChartDPX::ShouldRedraw()
{
    return true;
}
