#pragma once
#include <qpointer.h>
#include "Arks\Interfaces\base_impl\ark_base.h"
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
    SourceInfo									src_info_;
	QPointer<SpectralViewerWindow>				window_;
	std::shared_ptr<dpx_core::SpectrumDPX>      dpx_spectrum_;
	std::shared_ptr<spg_core::Spectrogram>      spectrogram_;
};

