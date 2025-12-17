#include "SpgChart.h"
using namespace spg_core;
ChartSPG::ChartSPG(QWidget * parrent, std::shared_ptr<SelectionHolder> selection_holder):
    ChartInterface(parrent, selection_holder)
{
    SetHorizontalMinMaxBounds({0, 1});
    SetHorizontalSuffix("counts");

    SetVerticalSuffix("power");
	scale_info_.val_info_.domain_type = ChartDomainType::kTimeFrequency;
	scale_info_.val_info_.max_zoom_koeffs_ = { 20, 1000};
	//SetBackgroundImage(":/AquaWave/third_party/background/black_forest.jpg");
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
}

void ChartSPG::PushData(const draw_data & draw_data)
{
    power_man_.UpdateBounds(draw_data.data, scale_info_.val_info_.min_max_bounds_.horizontal /*data_bounds*/);
    spg_core_.AccumulateNewData(draw_data.data,draw_data.time_pos);
}

void spg_core::ChartSPG::ClearData()
{
    spg_core_.Emplace();
	power_man_.ResetBounds();
}


void ChartSPG::SetVerticalMinMaxBounds(const Limits<double>& vert_bounds)
{
    ChartInterface::SetVerticalMinMaxBounds(vert_bounds);
	spg_core_.SetFreqBounds(vert_bounds);
}

void ChartSPG::SetHorizontalMinMaxBounds(const Limits<double>& hor_bounds)
{
	scale_info_.val_info_.max_zoom_koeffs_.horizontal = std::max(2., hor_bounds.delta() / 1000);
	ChartInterface::SetHorizontalMinMaxBounds(hor_bounds);	
    spg_core_.SetTimeBounds(hor_bounds);
}

void spg_core::ChartSPG::SetFftOrder(int fft_order)
{
	spg_core_.SetNfftOrder(fft_order);
}

spg_data const & ChartSPG::GetSpectrogramInfo() const
{
    return spg_core_.GetSpectrogramInfo();
}


bool ChartSPG::ShouldRedraw()
{
    return true;
}
