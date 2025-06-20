#include "SpgChart.h"

ChartSPG::ChartSPG(QWidget * parrent):
    ChartInterface(parrent)
{
    SetHorizontalMinMaxBounds(50'000, 200'000);
    SetHorizontalSuffix("counts");

    SetVerticalMinMaxBounds(10, 80, true);
    SetVerticalSuffix("power");
}

ChartSPG::~ChartSPG()
{

}

void ChartSPG::DrawData(QPainter & passed_painter)
{
    if(ShouldRedraw()) 
    {
        cached_pixmap_ = spg_painter_.GetRelevantPixmap(scale_info_);
    }
    if(cached_pixmap_.isNull()) 
        return;
    passed_painter.drawPixmap(0, 0, cached_pixmap_);
}

void ChartSPG::PushData(std::vector<float>& data, const Limits<double>& data_bounds)
{
    power_man_.UpdateBounds(data, scale_info_.val_info_.min_max_bounds_.horizontal /*data_bounds*/);
    spg_painter_.AccumulateNewData(data, scale_info_.val_info_.min_max_bounds_.horizontal);
}

void ChartSPG::SetVerticalMinMaxBounds(const double min_val, const double end_val, const bool is_adaptive)
{
    spg_painter_.SetTimeBounds({min_val, end_val});
    ChartInterface::SetVerticalMinMaxBounds(min_val, end_val, is_adaptive);
}


bool ChartSPG::ShouldRedraw()
{
    return true;
}
