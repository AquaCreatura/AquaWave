#pragma once
#include "..\ChartInterface.h"
class ChartSingle : public ChartInterface
{
public:
    ChartSingle(QWidget* parrent);
    virtual void DrawData   (QPainter& painter          ) override;
    virtual void PushData   (std::vector<float>& data, const Limits<double>& data_bounds) override;
    
};

