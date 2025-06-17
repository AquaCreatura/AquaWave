#pragma once
#include "GUI/Charts/ChartInterface.h"
#include "DPX core/DpxCore.h"

class ChartDPX : public ChartInterface
{
public:
    ChartDPX(QWidget* parrent);
    ~ChartDPX();
    virtual void DrawData                   (QPainter& painter          ) override;
    virtual void PushData                   (std::vector<float>& data, const Limits<double>& data_bounds) override;
protected:
    bool ShouldRedraw();
protected:
    dpx_core::DpxCore   dpx_painter_;
    QPixmap             cached_pixmap_;
    
};

