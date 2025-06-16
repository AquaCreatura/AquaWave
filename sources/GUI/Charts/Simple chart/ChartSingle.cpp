#include "ChartSingle.h"

ChartSingle::ChartSingle(QWidget * parrent):
    ChartInterface(parrent)
{
}

void ChartSingle::DrawData(QPainter & painter)
{
}

void ChartSingle::PushData(std::vector<float>& data, const Limits<double>& data_bounds)
{
}
