#pragma once
#include <qpointer.h>
#include "Arks\Interfaces\base_impl\ark_base.h"
#include "window\scope_analyzer_window.h"

#include "Elements/DPX Spectrum/SpectrumDPX.h"
#include "Elements/Static SPG/Spectrogram.h"
class ScopeAnalyzer : public fluctus::ArkBase
{
Q_OBJECT
public:
	ScopeAnalyzer();
	~ScopeAnalyzer();
    virtual bool		SendData   (fluctus::DataInfo const& data_info) override;
    virtual bool		SendDove   (fluctus::DoveSptr const & sent_dove) override;
    fluctus::ArkType	GetArkType () const override;
protected:
    bool Reload();
	bool Restart(fluctus::Limits<double> freq_bounds_hz, fluctus::Limits<double> time_bounds);
	void SetNewFftOrder(int n_fft_order);
protected slots:
    virtual void RequestSelectedData();

protected:
	fluctus::shared_vec<Ipp32fc>	data_;
	SourceInfo						src_info_;
	QPointer<ScopeAnalyzerWindow>	window_;
	double							freq_divider_ = 1.;
	int64_t							n_fft_{1024};

	std::shared_ptr<dpx_core::SpectrumDpx>      dpx_spectrum_;
	std::shared_ptr<spg_core::StaticSpg>		spg_;
};

