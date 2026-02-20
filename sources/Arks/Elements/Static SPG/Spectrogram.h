#pragma once
#include <qpointer.h>
#include "DSP Tools/Pipelines/BasePipes.h"
#include "Arks\Interfaces\base_impl\ark_base.h"
#include "GUI/Charts/SPG/helpers/SpgRequester.h"
#include "GUI/Charts/SPG/SpgChart.h"
#include "special_defs\spectral_viewer_defs.h"
namespace spg_core
{

class StaticSpg : public fluctus::ArkBase
{

public:
	StaticSpg(QWidget *parrent = nullptr);
    ~StaticSpg();
    virtual bool SendData(fluctus::DataInfo const& data_info) override;
    virtual bool SendDove(fluctus::DoveSptr const & sent_dove) override;
    ArkType GetArkType() const override;
protected:
    bool Reload();
protected:
    QPointer<ChartSPG>          spg_drawer_;
    SourceArk                  src_info_;
	WorkBounds                  time_bounds_;
    SpgRequester                requester_;
	std::shared_ptr<aqua_gui::SelectionHolder> selection_holder_;
	double						freq_divider_{ 1. };
	std::vector<std::shared_ptr<pipes::PipeInterface>>  dsp_pipes_;
};



}