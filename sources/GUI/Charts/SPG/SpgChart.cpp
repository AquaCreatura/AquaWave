#include "SpgChart.h"
using namespace spg_core;
ChartSPG::ChartSPG(QWidget * parrent):
    ChartInterface(parrent)
{
    SetHorizontalMinMaxBounds({50'000, 200'000});
    SetHorizontalSuffix("counts");

    SetPowerBounds({10, 80}, true);
    SetVerticalBounds({0,100});
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
    if(!cached_pixmap_.isNull()) 
        passed_painter.drawPixmap(0, 0, cached_pixmap_);
    emit NeedRequest(); //Ask for relevant data
}

void ChartSPG::PushData(const draw_data & draw_data)
{
    power_man_.UpdateBounds(draw_data.data, scale_info_.val_info_.min_max_bounds_.horizontal /*data_bounds*/);
    spg_core_.AccumulateNewData(draw_data.data,draw_data.time_pos);

    emit NeedRequest(); //Ask for relevant data
}

void spg_core::ChartSPG::ClearData()
{
    spg_core_.Emplace();
}


void ChartSPG::SetPowerBounds(const Limits<double>& power_bounds, const bool is_adaptive)
{
    ChartInterface::SetPowerBounds(power_bounds, is_adaptive);
}

void ChartSPG::SetHorizontalMinMaxBounds(const Limits<double>& hor_bounds)
{
    spg_core_.SetTimeBounds(hor_bounds);
}

spg_data const & ChartSPG::GetSpectrogramInfo() const
{
    return spg_core_.GetSpectrogramInfo();
}


bool ChartSPG::ShouldRedraw()
{
    return true;
}
