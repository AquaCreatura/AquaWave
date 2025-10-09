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
	~SpectrumDPX();
    virtual bool SendData   (fluctus::DataInfo const& data_info) override;
    virtual bool SendDove   (fluctus::DoveSptr const & sent_dove) override;
    ArkType      GetArkType () const override;
protected:
    bool Reload();
protected slots:
    virtual void RequestSelectedData();
protected:
    SourceInfo                  src_info_;
    QPointer<ChartDPX>          dpx_drawer_;
	QPointer<DpxWindow>			window_;
    FFT_Worker                  fft_worker_;
	double						freq_divider_ = 1.;
};

}