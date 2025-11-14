#pragma once
#include "GUI/Charts/ChartInterface.h"
#include "DPX core/DpxCore.h"

class ChartDPX : public ChartInterface
{
public:
    ChartDPX(QWidget* parrent = nullptr);
    ~ChartDPX();
    virtual void DrawData                   (QPainter& painter          ) override;
    virtual void PushData                   (const draw_data& draw_data ) override;
    virtual void ClearData                  ()                            override;
    virtual void SetPowerBounds             (const Limits<double>& power_bounds, const bool is_adaptive = true) override;
	virtual void SetHorizontalMinMaxBounds	(const Limits<double>& power_bounds) override;
	//requests for core
	void SetFftOrder(const int fft_order);
protected:
    bool ShouldRedraw();
protected:
    dpx_core::DpxCore   dpx_painter_;
    QPixmap             cached_pixmap_;
    
};

