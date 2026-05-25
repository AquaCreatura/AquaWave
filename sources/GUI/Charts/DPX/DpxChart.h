#pragma once
#include "GUI/Charts/ChartInterface.h"
#include "GUI/Tools/Chart tiler/ChartTiler.h"
using namespace aqua_gui;
class ChartDPX : public ChartInterface
{
public:
	ChartDPX(QWidget* parrent = nullptr, ChartDomainType domain = ChartDomainType::kFreqDomain , std::shared_ptr<SelectionHolder> selection_holder = {});
    ~ChartDPX();
    virtual void DrawData                   (QPainter& painter          ) override;
    virtual void PushData                   (const draw_data& draw_data ) override;
    virtual void ClearData                  ()                            override;
	virtual void SetHorizontalMinMaxBounds	(const Limits<double>& power_bounds) override;

	Limits<double> GetHorizontalMinMaxBounds();
public:
	void SetHorizontalDiscretisation (const size_t hor_discretisation);
protected:
	ChartTiler			tiler_;
};

