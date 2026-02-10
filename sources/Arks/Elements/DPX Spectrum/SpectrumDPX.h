#pragma once
#include <qpointer.h>
#include "Arks\Interfaces\base_impl\ark_base.h"
#include "GUI/Charts/DPX/DpxChart.h"
#include "DSP Tools/FFT/FFT_Worker.h"

#include "special_defs\spectral_viewer_defs.h"
namespace dpx_core
{

class SpectrumDpx : public fluctus::ArkBase
{
Q_OBJECT
public:
    SpectrumDpx();
	~SpectrumDpx();
    virtual bool SendData   (fluctus::DataInfo const& data_info) override;
    virtual bool SendDove   (fluctus::DoveSptr const & sent_dove) override;
    ArkType      GetArkType () const override;
protected:
    bool Reload();
	void SetNewFftOrder(int n_fft_order);
protected slots:
    virtual void RequestSelectedData();

protected:
    SourceArk                  src_info_;
	std::shared_ptr<aqua_gui::SelectionHolder> selection_holder_;
    QPointer<ChartDPX>          dpx_drawer_;
    FFT_Worker                  fft_worker_;
	double						freq_divider_ = 1.;
	int64_t						n_fft_{1024};
};

}