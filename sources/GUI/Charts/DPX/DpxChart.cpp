#include "DpxChart.h"

ChartDPX::ChartDPX(QWidget * parrent):
    ChartInterface(parrent)
{
    Limits<double> random_bounds = {50'000, 200'000};
    SetHorizontalMinMaxBounds(random_bounds);
    dpx_painter_.SetMinMax_X(random_bounds);
    SetHorizontalSuffix("counts");

    SetVerticalSuffix("power");
}

ChartDPX::~ChartDPX()
{
	SetVerticalSuffix("the end");
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

void ChartDPX::PushData(const draw_data& draw_data )
{

	bool need_reset = power_man_.NeedRelevantBounds();
	power_man_.UpdateBounds(draw_data.data, draw_data.freq_bounds);
	if (need_reset) {
		UpdateChartPowerBounds();
		dpx_painter_.SetPowerBounds(scale_info_.val_info_.min_max_bounds_.vertical);
	}
    dpx_painter_.AccumulateNewData(draw_data.data , draw_data.freq_bounds);
}

void ChartDPX::ClearData()
{
    dpx_painter_.Emplace();
	power_man_.ResetBounds();
	scale_info_.val_info_.need_reset_scale_ = true;
}

void ChartDPX::SetPowerBounds(const Limits<double>& power_bounds, const bool is_adaptive)
{
    dpx_painter_.SetPowerBounds(power_bounds);
    ChartInterface::SetPowerBounds(power_bounds, is_adaptive);
}

void ChartDPX::SetHorizontalMinMaxBounds(const Limits<double>& power_bounds)
{
	dpx_painter_.SetMinMax_X(power_bounds);
	ChartInterface::SetHorizontalMinMaxBounds(power_bounds);
}


bool ChartDPX::ShouldRedraw()
{
    return true;
}
