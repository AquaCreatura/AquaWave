#pragma once
#include <qpointer.h>
#include "Interfaces/base_impl/ark_base.h"
#include "Spectrogram\Spectrogram.h"
#include "DPX Spectrum\SpectrumDPX.h"
#include "Window/spectral_viewer_window.h"

class SpectralViewer : public fluctus::ArkBase
{
Q_OBJECT
public:
	SpectralViewer();
	~SpectralViewer();
    virtual bool SendData   (fluctus::DataInfo const& data_info) override;
    virtual bool SendDove   (fluctus::DoveSptr const & sent_dove) override;
    ArkType      GetArkType () const override;
protected:
    bool Reload();
	void SetNewFftOrder(int n_fft_order);
protected slots:
    virtual void RequestSelectedData();

protected:
    SourceInfo                  src_info_;

	QPointer<SpectralViewerWindow>	window_;
    FFT_Worker						fft_worker_;
	double							freq_divider_ = 1.;
	int64_t							n_fft_{1024};
};

