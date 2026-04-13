#pragma once
#include <qpointer.h>
#include "ark_defs.h"
#include "Arks/Interfaces/base_impl/ark_base.h"
#include "window/scope_analyzer_window.h"
#include "Tools/file_helpers.h"
#include "DSP Tools/Resampler/ResamplersImpl/MR Resampler/MultiRateResampler.h"

using namespace fluctus;
namespace scope_analyzer
{
class ScopeAnalyzer : public fluctus::ArkBase
{
	Q_OBJECT
public:
	ScopeAnalyzer();
	~ScopeAnalyzer();
	virtual bool		SendData(fluctus::DataInfo const& data_info) override;
	virtual bool		SendDove(fluctus::DoveSptr const & sent_dove) override;
	fluctus::ArkType	GetArkType() const override;
protected:
	bool Reload();
	bool Restart(fluctus::Limits<double> freq_bounds_hz, fluctus::Limits<double> time_bounds);
	void SetNewFftOrder(int n_fft_order);
protected:
	struct chart_info : ArkInterface::sptr{
		chart_info& operator=(const ArkInterface::sptr& other) {
			ArkInterface::sptr::operator=(other);
			return *this;
		}
		bool				need_resampler = true;
	};
	SourceArk						source_info_;
	Limits<double>					time_bounds_;
	SourceDescription				selection_descr_;
	int64_t							resampled_samplerate_;

	QPointer<ScopeAnalyzerWindow>	window_;
	int64_t							n_fft_{ 1024 * 2 };
	std::map<scope_chart_type, chart_info> charts_;
	FileWriter						cur_writer_;

	aqua_resampler::MultiRateResampler	resampler_;
	fluctus::DataInfo					resampled_unit_;
	std::vector<Ipp32fc>				resampled_buff_;
};



}