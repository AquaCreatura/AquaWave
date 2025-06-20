#pragma once
#include "GUI/Charts/ChartInterface.h"
#include "SPG core/SpgCore.h"

class ChartSPG : public ChartInterface
{
public:
    ChartSPG(QWidget* parrent);
    ~ChartSPG();
    virtual void DrawData                   (QPainter& painter          ) override;
    virtual void PushData                   (std::vector<float>& data, const Limits<double>& data_bounds) override;
    virtual void SetVerticalMinMaxBounds    (const double min_val, const double end_val, const bool is_adaptive = true);
protected:
    bool ShouldRedraw();
protected:
   
    spg_core::SpgCore   spg_painter_;
    QPixmap             cached_pixmap_;
    
};

