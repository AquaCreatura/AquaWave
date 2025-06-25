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
    virtual void SetVerticalMinMaxBounds    (const double min_val, const double end_val, const bool is_adaptive = true);
    virtual void SetHorizontalMinMaxBounds  (const double min_val  , const double end_val);

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