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
    // �������� ������� "��������������" ������� �������� �� ����������� DPX
    auto new_bounds = dpx_painter_.GetPowerBounds();
    // �������� ������ �� ������� ����������� ���������� (��������������) ������� �����
    auto &vert_min_max = scale_info_.val_info_.min_max_bounds_.vertical;

    // ���� �������������� ������� ����������, ��������� �����
    if(vert_min_max != new_bounds)
    {
        // �������� ������ �� ������� ������������ ������� ����� (������� ����� ������������)
        auto &vert_cur               = scale_info_.val_info_.cur_bounds.vertical;
        // ��������� ������� ����������� ��������������� (����) �� ���������
        const double zoom_vert_koeff = vert_cur.delta() /  vert_min_max.delta();
        // ������������ ����� ������ (��������) ��� ������������ �����, �������� ���
        const double new_height      = (new_bounds.high - new_bounds.low) * zoom_vert_koeff;
        // ��������� ������� ����� ������������ �����
        double zoom_centre           = (vert_cur.high + vert_cur.low) / 2;
        // ������������ ����� ����, ����� ������������ �������� �� ����� �� ����� �������������� �������
        zoom_centre = qBound(new_bounds.low + new_height / 2, zoom_centre, new_bounds.high - new_height / 2);

        // ��������� ����������� ���������� (��������������) �������
        vert_min_max = new_bounds;
        // ��������� ������� ������������ ������� � ������ ������ ������ � ������
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
