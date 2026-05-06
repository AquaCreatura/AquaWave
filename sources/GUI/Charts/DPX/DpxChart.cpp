#include "DpxChart.h"
#include "GUI/Tools/gui_worker.h"
ChartDPX::ChartDPX(QWidget * parrent, ChartDomainType domain, std::shared_ptr<SelectionHolder> selection_holder):
    ChartInterface(parrent, selection_holder, domain), tiler_(scale_info_)
{
    Limits<double> random_bounds = {0, 100};
    SetHorizontalMinMaxBounds(random_bounds);
    //dpx_painter_.SetMinMax_X(random_bounds);
    SetHorizontalSuffix("counts");

    SetVerticalSuffix("power");
	//SetBackgroundImage(":/AquaWave/third_party/background/dark_sky.jpg");
}

ChartDPX::~ChartDPX()
{
	SetVerticalSuffix("the end");
}

void ChartDPX::DrawData(QPainter & passed_painter)
{
    if(ShouldRedraw()) 
    {
        cached_pixmap_ = tiler_.GetRelevantPixmap();
    }
    if(cached_pixmap_.isNull()) 
        return;
    passed_painter.drawPixmap(0, 0, cached_pixmap_);
}

void ChartDPX::PushData(const draw_data& draw_data )
{
	//Обновляем порог по мощности
	{
		power_man_.UpdateBounds(draw_data.data, draw_data.freq_bounds);
		scale_info_.power_bounds_ = power_man_.GetPowerBounds();
		if (power_man_.NeedRelevantBounds()) {
			aqua_gui::AdaptVertPowerBounds(scale_info_);
			tiler_.UpdateBounds();
		}
	}
	tiler_.SetData(draw_data);
}

void ChartDPX::ClearData()
{
	tiler_.Reset();
	power_man_.ResetBounds();
	scale_info_.val_info_.need_reset_scale = true;
}


void ChartDPX::SetHorizontalMinMaxBounds(const Limits<double>& power_bounds)
{
	tiler_.Reset();
	ChartInterface::SetHorizontalMinMaxBounds(power_bounds);
}

Limits<double> ChartDPX::GetHorizontalMinMaxBounds()
{
	return scale_info_.val_info_.min_max_bounds.hor;
}

void ChartDPX::SetHorizontalDiscretisation(const size_t hor_discretisation)
{
	scale_info_.val_info_.max_zoom_koeffs.hor = std::max(1., hor_discretisation / 20.);
}



bool ChartDPX::ShouldRedraw()
{
    return true;
}
