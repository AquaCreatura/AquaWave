#pragma once
#include "Interfaces/base_impl/ark_base.h"
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

protected:
    std::shared_ptr<ChartSPG> dpx_drawer_;
    FFT_Worker fft_worker_;
};

}