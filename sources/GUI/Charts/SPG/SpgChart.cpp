#include "SpgChart.h"
using namespace spg_core;
ChartSPG::ChartSPG(QWidget * parrent):
    ChartInterface(parrent)
{
    SetHorizontalMinMaxBounds(50'000, 200'000);
    SetHorizontalSuffix("counts");

    SetVerticalMinMaxBounds(10, 80, true);
    SetVerticalSuffix("power");
    domain_type_ = ChartDomainType::kTimeFrequency;
}

ChartSPG::~ChartSPG()
{

}

void ChartSPG::DrawData(QPainter & passed_painter)
{
    if(ShouldRedraw()) 
    {
        cached_pixmap_ = spg_core_.GetRelevantPixmap(scale_info_);
    }
    if(cached_pixmap_.isNull()) 
        return;
    passed_painter.drawPixmap(0, 0, cached_pixmap_);
}

void ChartSPG::PushData(const draw_data & draw_data)
{
    power_man_.UpdateBounds(draw_data.data, scale_info_.val_info_.min_max_bounds_.horizontal /*data_bounds*/);
    spg_core_.AccumulateNewData(draw_data.data,draw_data.time_pos);
}


void ChartSPG::SetVerticalMinMaxBounds(const double min_val, const double end_val, const bool is_adaptive)
{
    ChartInterface::SetVerticalMinMaxBounds(min_val, end_val, is_adaptive);
}

void ChartSPG::SetHorizontalMinMaxBounds(const double min_val, const double end_val)
{
    spg_core_.SetTimeBounds({min_val, end_val});
}

spg_data const & ChartSPG::GetSpectrogramInfo() const
{
    return spg_core_.GetSpectrogramInfo();
}


bool ChartSPG::ShouldRedraw()
{
    return true;
}
