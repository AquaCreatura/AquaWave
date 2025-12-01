#pragma once
#include <qpointer.h>
#include "Arks\Interfaces\base_impl\ark_base.h"
#include "window\scope_analyzer_window.h"

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
	void SetNewFftOrder(int n_fft_order);
protected slots:
    virtual void RequestSelectedData();

protected:

	QPointer<ScopeAnalyzerWindow>	window_;
	double							freq_divider_ = 1.;
	int64_t							n_fft_{1024};
};

