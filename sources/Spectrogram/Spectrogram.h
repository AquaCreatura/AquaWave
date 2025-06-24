#pragma once
#include "Interfaces/base_impl/ark_base.h"
#include "SpgRequester.h"
#include "GUI/Charts/SPG/SpgChart.h"
#include "DSP Tools/FFT/FFT_Worker.h"
namespace spg_core
{

class Spectrogram : public fluctus::ArkBase
{

public:
    Spectrogram(QWidget *parrent);
    virtual bool SendData(fluctus::DataInfo const& data_info) override;
    virtual bool SendDove(fluctus::DoveSptr const & sent_dove) override;

protected:
    std::shared_ptr<ChartSPG> spg_drawer_;
    WorkBounds                time_bounds_;
    FFT_Worker fft_worker_;
    SpgRequester requester_;
};

}