#pragma once
#include "Arks/Interfaces/base_impl/ark_base.h"
#include "DSP Tools/Pipelines/BasePipes.h"
#include "GUI/Charts/SPG/SpgChart.h"
#include "requester/SpgRequester.h"

#include <qpointer.h>

namespace spg_core
{

class StaticSpg : public aqua::ArkBase
{

public:
	StaticSpg(QWidget *parrent = nullptr);
    ~StaticSpg();
    virtual bool SendData(aqua::DataInfo const& data_info) override;
    virtual bool PostDove(aqua::DoveSptr const & sent_dove) override;
    ArkType GetArkType() const override;
protected:
    bool Reload();
protected:
    QPointer<ChartSPG>          spg_drawer_;
    SourceArk					src_info_;
    SpgRequester                requester_;
	std::shared_ptr<aqua_gui::SelectionHolder> selection_holder_;
	double						freq_divider_{ 1. };
	pipes::SimplePipeLine		pipe_line_;
};



}
