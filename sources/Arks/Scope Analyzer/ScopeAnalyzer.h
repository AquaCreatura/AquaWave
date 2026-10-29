#pragma once
#include "Arks/Interfaces/base_impl/ark_base.h"
#include "DSP Tools/Localiser/HarmonicEstimator.h"
#include "DSP Tools/Resampler/ResamplersImpl/MR Resampler/MultiRateResampler.h"
#include "Utilities/file_helpers.h"
#include "window/scope_analyzer_window.h"

#include <qpointer.h>

using namespace aqua;
using namespace analyzer;
namespace scope_analyzer
{
class ScopeAnalyzer : public aqua::ArkBase
{
	Q_OBJECT
public:
	ScopeAnalyzer();
	~ScopeAnalyzer();
	virtual bool		SendData(aqua::DataInfo const& data_info) override;
	virtual bool		PostDove(aqua::DoveSptr const & sent_dove) override;
	aqua::ArkType	GetArkType() const override;
protected:
	void ActivateNode(const bool do_activate);
	void UpdateHarmonicInfo();
	bool Reload();
	bool Restart();
	void SetNewFftOrder(int n_fft_order);
	void SetDemodParam(aqua_opt<double> carrier_hz, aqua_opt<double> symbolrate_hz);
protected:
	struct chart_info : ArkInterface::sptr{
		chart_info& operator=(const ArkInterface::sptr& other) {
			ArkInterface::sptr::operator=(other);
			return *this;
		}
		bool				need_resampler = true;
	};
	SourceArk						source_info_;
	Limits<double>					selection_time_bounds_;
	Limits<double>					selection_freq_bounds_;
	SourceDescription				selection_descr_;
	int64_t							resampled_samplerate_;
	QTimer							info_timer_;
	bool							is_active_{ false };

	QPointer<ScopeAnalyzerWindow>	window_;
	HarmonicEstimator				estimator_;
	int64_t							n_fft_{ 1024 * 2 };
	std::map<analz_chart_type, chart_info> charts_;
	FileWriter						cur_writer_;

	aqua_resampler::MultiRateResampler	resampler_;
	aqua::DataInfo					resampled_unit_;
	std::vector<Ipp32fc>				resampled_buff_;
	bool							inner_attached_{ false };
};



}
