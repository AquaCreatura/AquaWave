#pragma once
#include "GUI/Charts/ChartInterface.h"
#include "SPG core/SpgCore.h"
namespace spg_core
{
class ChartSPG : public ChartInterface
{
    Q_OBJECT 
public:
	ChartSPG(QWidget* parrent = nullptr, std::shared_ptr<SelectionHolder> selection_holder = {});
    ~ChartSPG();
    virtual void DrawData                   (QPainter& painter          ) override;
    virtual void PushData                   (const draw_data& draw_data ) override;
    virtual void ClearData                  ()                            override;
    virtual void SetVerticalMinMaxBounds	(const Limits<double>& vert_bounds) override;
    virtual void SetHorizontalMinMaxBounds  (const Limits<double>& hor_bounds) override;

//Requests for core
	void			 SetFftOrder(int fft_order);
    spg_data const & GetSpectrogramInfo() const;
protected:
    bool ShouldRedraw();
	void ChangeTimeDomain();
protected:
	bool				is_counts_mode_{ true };
    spg_core::SpgCore   spg_core_;
    QPixmap             cached_pixmap_;
    
};

}