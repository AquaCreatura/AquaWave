#include "DpxChart.h"

ChartDPX::ChartDPX(QWidget * parrent):
    ChartInterface(parrent)
{
    SetHorizontalMinMaxBounds(50'000, 200'000);
    SetHorizontalSuffix("counts");

    SetVerticalMinMaxBounds(0, 50, true);
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
    dpx_painter_.PassNewData(data, scale_info_.val_info_.min_max_bounds_.horizontal);
}

void ChartDPX::UpdatePowerBounds()
{
    // ѕолучаем текущие "автоматические" границы мощности от отрисовщика DPX
    auto new_bounds = dpx_painter_.GetPowerBounds();
    // ѕолучаем ссылку на текущие максимально допустимые (автоматические) границы шкалы
    auto &vert_min_max = scale_info_.val_info_.min_max_bounds_.vertical;

    // ≈сли автоматические границы изменились, обновл€ем шкалу
    if(vert_min_max != new_bounds)
    {
        // ѕолучаем ссылку на текущие отображаемые границы шкалы (которые видит пользователь)
        auto &vert_cur               = scale_info_.val_info_.cur_bounds.vertical;
        // ¬ычисл€ем текущий коэффициент масштабировани€ (зума) по вертикали
        const double zoom_vert_koeff = vert_cur.delta() /  vert_min_max.delta();
        // –ассчитываем новую высоту (диапазон) дл€ отображаемой шкалы, сохран€€ зум
        const double new_height      = (new_bounds.high - new_bounds.low) * zoom_vert_koeff;
        // ¬ычисл€ем текущий центр отображаемой шкалы
        double zoom_centre           = (vert_cur.high + vert_cur.low) / 2;
        //  орректируем центр зума, чтобы отображаемый диапазон не вышел за новые автоматические границы
        zoom_centre = qBound(new_bounds.low + new_height / 2, zoom_centre, new_bounds.high - new_height / 2);

        // ќбновл€ем максимально допустимые (автоматические) границы
        vert_min_max = new_bounds;
        // ќбновл€ем текущие отображаемые границы с учетом нового центра и высоты
        vert_cur     = {zoom_centre - new_height/2, zoom_centre + new_height/2};
    }
}

void ChartDPX::SetVerticalMinMaxBounds(const double min_val, const double end_val, const bool is_adaptive)
{
    dpx_painter_.SetPowerBounds({min_val, end_val}, is_adaptive);
    ChartInterface::SetVerticalMinMaxBounds(min_val, end_val, is_adaptive);
}

bool ChartDPX::ShouldRedraw()
{
    return true;
}
