#pragma once
#include <qpointer.h>
#include "Interfaces/base_impl/ark_base.h"
#include "GUI/Charts/DPX/DpxChart.h"
#include "window/DpxWindow.h"
#include "DSP Tools/FFT/FFT_Worker.h"
namespace dpx_core
{

class SpectrumDPX : public fluctus::ArkBase
{
Q_OBJECT
public:
    SpectrumDPX();
    virtual bool SendData   (fluctus::DataInfo const& data_info) override;
    virtual bool SendDove   (fluctus::DoveSptr const & sent_dove) override;
    ArkType      GetArkType () const override;
protected:
protected slots:
    virtual void OnDoSomething();
protected:
    QPointer<ChartDPX>          dpx_drawer_;
    std::shared_ptr<DpxWindow>  window_;
    FFT_Worker                  fft_worker_;
};

}