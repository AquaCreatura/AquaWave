#include "SpgChart.h"
#include <qshortcut.h>
using namespace spg_core;
ChartSPG::ChartSPG(QWidget * parrent, std::shared_ptr<SelectionHolder> selection_holder):
    ChartInterface(parrent, selection_holder), tiler_(scale_info_)
{
    SetHorizontalMinMaxBounds({0, 1});
    SetHorizontalSuffix("counts");

    SetVerticalSuffix("power");
	scale_info_.val_info_.domain_type = ChartDomainType::kTimeFrequency;
	//SetBackgroundImage(":/AquaWave/third_party/background/black_forest.jpg");


	auto* shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), this);
	connect(shortcut, &QShortcut::activated, this, &ChartSPG::ChangeTimeDomain);
}

ChartSPG::~ChartSPG()
{
	printf_s("ChartSPG Destroyed...");
}

void ChartSPG::DrawData(QPainter & passed_painter)
{
	auto relevant_pixmap = tiler_.GetRelevantPixmap();
	if (relevant_pixmap.isNull()) return;
	passed_painter.drawPixmap(0, 0, relevant_pixmap);
}

void ChartSPG::PushData(const draw_data & draw_data)
{
    power_man_.UpdateBounds(draw_data.data, scale_info_.val_info_.min_max_bounds.hor /*data_bounds*/);
	tiler_.SetData(draw_data);
}

void spg_core::ChartSPG::ClearData()
{
	tiler_.Reset();
	power_man_.ResetBounds();
}


void ChartSPG::SetVerticalMinMaxBounds(const Limits<double>& vert_bounds)
{
    ChartInterface::SetVerticalMinMaxBounds(vert_bounds);
	tiler_.Reset();
}

void ChartSPG::SetHorizontalMinMaxBounds(const Limits<double>& hor_bounds)
{
	if (!is_counts_mode_) ChangeTimeDomain();
	scale_info_.val_info_.max_zoom_koeffs.hor = std::max(2., hor_bounds.delta() / 1000);
	ChartInterface::SetHorizontalMinMaxBounds(hor_bounds);	
	tiler_.Reset();
}

void spg_core::ChartSPG::SetFftOrder(int fft_order)
{
	scale_info_.val_info_.max_zoom_koeffs.vert = std::max(1., (1 << fft_order) / 20.);
	tiler_.Reset();
}

ChartTiler const & spg_core::ChartSPG::GetTiler() const
{
	return tiler_;
}




void spg_core::ChartSPG::ChangeTimeDomain()
{
	const auto need_count_mode = !is_counts_mode_;
	auto& min_max = scale_info_.val_info_.min_max_bounds;
	const double freq_bounds_hz = min_max.vert.delta() * 1e6;
	const double multiply_koeff = need_count_mode ? freq_bounds_hz : 1. / freq_bounds_hz;
	min_max.hor  = min_max.hor * multiply_koeff;
	scale_info_.val_info_.view_bounds.hor = scale_info_.val_info_.view_bounds.hor * multiply_koeff;

	is_counts_mode_ = need_count_mode;
	if (is_counts_mode_) scale_info_.val_info_.max_zoom_koeffs.hor = std::max(2., min_max.hor.delta() / 1000);
	
	//spg_core_.SetTimeBounds(min_max.hor);
	SetHorizontalSuffix(is_counts_mode_ ? "counts" : "time");
}
