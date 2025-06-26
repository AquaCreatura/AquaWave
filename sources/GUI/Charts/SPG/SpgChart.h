#pragma once
#include "GUI/Charts/ChartInterface.h"
#include "SPG core/SpgCore.h"
namespace spg_core
{
class ChartSPG : public ChartInterface
{
    Q_OBJECT 
public:
    ChartSPG(QWidget* parrent = nullptr);
    ~ChartSPG();
    virtual void DrawData                   (QPainter& painter          ) override;
    virtual void PushData                   (const draw_data& draw_data ) override;
    virtual void ClearData                  ()                            override;
    virtual void SetPowerBounds             (const Limits<double>& power_bounds, const bool is_adaptive = true) override;
    virtual void SetHorizontalMinMaxBounds  (const Limits<double>& hor_bounds) override;
    spg_data const & GetSpectrogramInfo() const;
signals:
    void NeedRequest();
protected:
    bool ShouldRedraw();
protected:
   
    spg_core::SpgCore   spg_core_;
    QPixmap             cached_pixmap_;
    
};

}