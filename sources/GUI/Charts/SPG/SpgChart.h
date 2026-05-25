#pragma once
#include "GUI/Charts/ChartInterface.h"
#include "GUI/Tools/Chart tiler/ChartTiler.h"
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
	ChartTiler const &GetTiler() const;

protected:
	void ChangeTimeDomain();
protected:
	bool				is_counts_mode_{ true };
	ChartTiler			tiler_;
    
};

}