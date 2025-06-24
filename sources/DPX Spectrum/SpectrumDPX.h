#pragma once
#include "Interfaces/base_impl/ark_base.h"
#include "GUI/Charts/DPX/DpxChart.h"
#include "DSP Tools/FFT/FFT_Worker.h"
namespace dpx_core
{

class SpectrumDPX : public fluctus::ArkBase
{

public:
    SpectrumDPX(QWidget *parrent);
    virtual bool SendData   (fluctus::DataInfo const& data_info) override;
    virtual bool SendDove   (fluctus::DoveSptr const & sent_dove) override;
    ArkType      GetArkType () const override;
protected:

protected:
    std::shared_ptr<ChartDPX> dpx_drawer_;
    FFT_Worker fft_worker_;
};

}