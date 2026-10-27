#include "ScopeAnalyzer.h"
#include "special_defs/file_souce_defs.h"
#include "special_defs/analyzer_defs.h"
#include "special_defs/demodulator_defs.h"
#include "Scheme/ShipBuilder.h"
#include <ippvm.h>

#include <qdebug.h>
#include <qmessagebox.h>

#include "Elements/DPX Spectrum/SpectrumDPX.h"
#include "Elements/Static SPG/Spectrogram.h"
#include "Elements/Constellation/Constellation.h"

using namespace aqua;
using namespace scope_analyzer;
using namespace spectral_viewer;
using namespace analyzer;
ScopeAnalyzer::ScopeAnalyzer()
{
    window_ = new ScopeAnalyzerWindow;

	//Определяем графики гармонического анализа
	{
		charts_[analz_chart_type::kAcf]			 = std::make_shared<dpx_core::SpectrumDpx>(dpx_core::kDpxChartType::kACF);
		charts_[analz_chart_type::kAcf].need_resampler = false;

		charts_[analz_chart_type::kConstellation] = std::make_shared<constel::Constellation>();
		charts_[analz_chart_type::kConstellation].need_resampler = false;

		charts_[analz_chart_type::kPhasorSpectrum]	= std::make_shared<dpx_core::SpectrumDpx>(dpx_core::kDpxChartType::kPhasor);
		charts_[analz_chart_type::kEnvelopeSpectrum]	= std::make_shared<dpx_core::SpectrumDpx>(dpx_core::kDpxChartType::kEnvelope);
		charts_[analz_chart_type::kPowerSpectrum]		= std::make_shared<dpx_core::SpectrumDpx>(dpx_core::kDpxChartType::kPower4x);
		
		for (auto chart_type : { analz_chart_type::kAcf, analz_chart_type::kPowerSpectrum , analz_chart_type::kPhasorSpectrum, analz_chart_type::kEnvelopeSpectrum, analz_chart_type::kConstellation }) {
			auto chart_window = ShipBuilder::GetWindow(charts_[chart_type]);
			window_->AddChartWindow(chart_window, chart_type);
		}
	}
	window_->ActivateWindow(analz_chart_type::kPowerSpectrum);
	resampler_.SetPrecision(0);
	connect(window_, &ScopeAnalyzerWindow::FftChangeNeed, this, &ScopeAnalyzer::SetNewFftOrder);
	info_timer_.setInterval(1000); // Интервал 1 секунда (1000 мс), задается один раз
	info_timer_.callOnTimeout(this, &ScopeAnalyzer::UpdateHarmonicInfo);

}

ScopeAnalyzer::~ScopeAnalyzer()
{
	qDebug() << "Scope analyzer destroyed! \n";
}

bool ScopeAnalyzer::SendData(aqua::DataInfo const & passed_unit)
{
	if (!is_active_)
		return false;

	{
		static auto last_time = std::chrono::steady_clock::now();
		static int call_count = 0;

		call_count++;

		auto now = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_time).count();

		if (elapsed >= 1) {
			qDebug() << "SendData calls per second:" << call_count << "\n";
			call_count = 0;
			last_time = now;
		}

	}
	int cur_block_size = n_fft_;
	if (passed_unit.data_vec.size() != cur_block_size * sizeof(Ipp32fc))
		return false;
	for (auto chart_iter : charts_) {
		if(!chart_iter.second.need_resampler)
			chart_iter.second->SendData(passed_unit);
	}

	//Обработка с повышением ЧД
	{
		auto &resampled = (std::vector<Ipp32fc>&)resampled_unit_.data_vec;
		resampler_.ProcessData((std::vector<Ipp32fc>&)passed_unit.data_vec, resampled_buff_);
		resampled_unit_.freq_info_.carrier_hz = passed_unit.freq_info_.carrier_hz;
		const int count_of_parts = resampled_buff_.size() / cur_block_size;
		for (int part_counter = 0; part_counter < count_of_parts; part_counter++) {
			resampled.assign(resampled_buff_.data() + cur_block_size * part_counter, resampled_buff_.data() + cur_block_size * (part_counter + 1));
			for (auto chart_iter : charts_) {
				if (chart_iter.second.need_resampler)
					chart_iter.second->SendData(resampled_unit_);
			}
		}
	}

	return true;
}


// Обрабатывает сообщения "Dove".
// sent_dove: Умный указатель на сообщение Dove.
bool ScopeAnalyzer::PostDove(aqua::DoveSptr const & sent_dove)
{
    // Если сообщение недействительно, выбрасываем исключение.
    if (!sent_dove) throw std::invalid_argument("Not created message sent!");
    
    // Получаем целевое значение и "мысль" из сообщения.
    auto target_val = sent_dove->target_ark;
    auto base_thought = sent_dove->base_thought;
    
    // Если "мысль" - запрос на диалог.
    if (base_thought & aqua::DoveParrent::DoveThought::kGetWindow)
    {
        // Прикрепляем отрисовщик спектра к виджету сообщения.
        sent_dove->show_widget = window_;
        return true; // Запрос обработан.
    }
    if(base_thought == aqua::DoveParrent::DoveThought::kAddSource)
    {
        if( target_val->GetArkType() == ArkType::kFileSource) 
			source_info_.ark = target_val;
        Reload();
    }
    //
    if(base_thought == aqua::DoveParrent::DoveThought::kReset)
    {
        return Reload();
    }
	if (base_thought & aqua::DoveParrent::DoveThought::kActivate)
	{
		ActivateNode(true);
	}
	if (base_thought & aqua::DoveParrent::DoveThought::kDeactivate)
	{
		ActivateNode(false);
	}

	if (base_thought & aqua::DoveParrent::DoveThought::kGetDescription)
	{
		sent_dove->description = selection_descr_;
		for (auto chart_iter : charts_) {
			if ((chart_iter.second == sent_dove->sender) && (chart_iter.second.need_resampler)) {
				sent_dove->description->samplerate_hz = resampled_samplerate_;
			}
		}
		
	}
	if (base_thought == aqua::DoveParrent::DoveThought::kSpecialThought) {
		const auto special_thought = sent_dove->special_thought;
		if (auto spectral_dove = std::dynamic_pointer_cast<analyzer::AnalyzeDove>(sent_dove)) {

			if (special_thought & analyzer::AnalyzeDove::kInitWithFile) {
				selection_time_bounds_ = spectral_dove->file_bounds_ratio;
				selection_freq_bounds_ = spectral_dove->freq_bounds_hz * 1.e6;
				if (is_active_) ActivateNode(true);
			}
		};
		if (auto file_src_dove = std::dynamic_pointer_cast<file_source::FileSrcDove>(sent_dove)) {

		}

	}
    // Передаём сообщение базовому классу для дальнейшей обработки.
    return ArkBase::PostDove(sent_dove);
}

ArkType ScopeAnalyzer::GetArkType() const
{
    return ArkType::kScopeAnalyser;
}
void scope_analyzer::ScopeAnalyzer::ActivateNode(const bool do_activate)
{	
	window_->ActivateWindow(do_activate ? window_->GetCurrentChart() : analz_chart_type::undefined);
	if (do_activate) {		
		info_timer_.start();
		Restart();
	}
	else {
		info_timer_.stop();
		if (auto file_src = source_info_.ark.lock()) {
			auto req_dove = std::make_shared<DoveParrent>(file_source::FileSrcDove::kStop);
			req_dove->target_ark = shared_from_this();
			file_src->PostDove(req_dove);
		}
	}
	is_active_ = do_activate;
}
void scope_analyzer::ScopeAnalyzer::UpdateHarmonicInfo()
{
	auto req_dove = std::make_shared<analyzer::AnalyzeDove>(analyzer::AnalyzeDove::kGetHarmonicResult);
	for (auto chart_iter : charts_) {		
		chart_iter.second->PostDove(req_dove);
		if (req_dove->peaks) {
			auto &vec_of_peaks = *req_dove->peaks;	
			estimator_.SetLocalisedPeaks(chart_iter.first, *req_dove->peaks);
		}		
	}
	{
		estimator_.Estimate();
		window_->UpdateHarmonicResult(analz_chart_type::kAcf,				estimator_.GetAcfMsec().value_or(0.));
		window_->UpdateHarmonicResult(analz_chart_type::kPhasorSpectrum,	estimator_.GetSymbolRateHz().value_or(0.));
		window_->UpdateHarmonicResult(analz_chart_type::kEnvelopeSpectrum,	estimator_.GetSymbolRateHz().value_or(0.));
		window_->UpdateHarmonicResult(analz_chart_type::kPowerSpectrum,		estimator_.GetCarrierHz().value_or(0.));

		req_dove->symbol_rate_hz = estimator_.GetSymbolRateHz();
		req_dove->carrier_hz	 = estimator_.GetCarrierHz();
	}
	//

	req_dove->special_thought = analyzer::AnalyzeDove::kSetHarmonicInfo;
	if (req_dove->symbol_rate_hz && req_dove->carrier_hz) {
		charts_[analz_chart_type::kConstellation]->PostDove(req_dove);
	}
	SetDemodParam(estimator_.GetCarrierHz(), estimator_.GetSymbolRateHz());
}

bool ScopeAnalyzer::Reload()
{
	auto file_src = source_info_.ark.lock();

	if (!file_src) return true;	
	{
		auto req_dove = std::make_shared<aqua::DoveParrent>(aqua::DoveParrent::kGetDescription);
		if (!file_src->PostDove(req_dove) || !req_dove->description) {
			return false;
		}
		source_info_.descr = *req_dove->description;
	}
	return true;
}

bool ScopeAnalyzer::Restart()
{
	auto file_src = source_info_.ark.lock();
	if (selection_time_bounds_.delta() == 0) return false;
	if (!file_src) return false;

	selection_descr_.bw_ratio_ = source_info_.descr.bw_ratio_;
	selection_descr_.carrier_hz = selection_freq_bounds_.mid();



	{
		auto req_dove = std::make_shared<file_source::FileSrcDove>();
		req_dove->special_thought = file_source::FileSrcDove::kInitiate | file_source::FileSrcDove::kAskLoopInRange;
		req_dove->target_ark = shared_from_this();

		req_dove->time_bounds = selection_time_bounds_;

		req_dove->setup.emplace();
		auto &setup = req_dove->setup;
		setup->carrier_hz = selection_descr_.carrier_hz;
		setup->chunk_size = n_fft_;		
		setup->banwidth_hz	 = selection_freq_bounds_.delta();
		setup->samplerate_hz = setup->banwidth_hz / source_info_.descr.bw_ratio_;

		if (!file_src->PostDove(req_dove))
		{
			QMessageBox::warning( nullptr, "Cannot Send Data", "Do something with DPX or file source, or...");
			return false;
		}
		selection_descr_.samplerate_hz = setup->samplerate_hz; //Выставляем ЧД
		resampled_samplerate_ = setup->banwidth_hz * 2;
		resampler_.Init(setup->samplerate_hz, resampled_samplerate_, setup->samplerate_hz);
		resampled_unit_.freq_info_.samplerate_hz = resampled_samplerate_;
	}
	selection_descr_.count_of_samples = selection_time_bounds_.delta() * source_info_.descr.count_of_samples *
											selection_descr_.samplerate_hz / source_info_.descr.samplerate_hz;


	estimator_.Reset();
	estimator_.SetBaseCarrier(selection_descr_.carrier_hz);
	estimator_.SetBandwidth(selection_descr_.samplerate_hz);

	if (!inner_attached_) {
		for (auto chart_iter : charts_)
			ShipBuilder::Bind_SrcSink(shared_from_this(), chart_iter.second);
		inner_attached_ = true;
	}
	

	const int max_order = std::min(log2(selection_descr_.count_of_samples), 21.);

	auto req_dove = std::make_shared<spectral_viewer::SpectralDove>();
	req_dove->base_thought = DoveParrent::kReset;
	for (auto chart_iter : charts_)
		chart_iter.second->PostDove(req_dove);

	if(max_order > 0)
		window_->SetMaxFFtOrder(max_order);

	
	return true;
}

void scope_analyzer::ScopeAnalyzer::SetNewFftOrder(int need_order)
{
	auto arks = GetSourceArks();
	if (arks.empty()) return;

	n_fft_ = 1 << need_order;
	{
		auto req_dove = std::make_shared<spectral_viewer::SpectralDove>();
		req_dove->base_thought = DoveParrent::kSpecialThought;
		req_dove->special_thought = spectral_viewer::SpectralDove::kSetFFtOrder;
		req_dove->fft_order_ = log2(n_fft_);
		for (auto chart_iter : charts_) 
			chart_iter.second->PostDove(req_dove);

	}
	if (selection_time_bounds_.delta() == 0) return;
	auto file_src_ = arks.front();
	auto req_dove = std::make_shared<file_source::FileSrcDove>();
	req_dove->special_thought = file_source::FileSrcDove::kSetChunkSize;
	req_dove->target_ark = shared_from_this();	
	req_dove->setup.emplace()->chunk_size = n_fft_;
	if (!file_src_->PostDove(req_dove))
	{
		QMessageBox::warning(nullptr, "Cannot Send Data", "Do something with DPX or file source, or...");
		return;
	}


	return;
}

void scope_analyzer::ScopeAnalyzer::SetDemodParam(aqua_opt<double> carrier_hz, aqua_opt<double> symbolrate_hz)
{
	
	auto front_arks = GetCmdArks();
	for (auto front_iter : front_arks) {
		if (front_iter->GetArkType() == aqua::kCcmDemodulator) {
			auto demod_dove = std::make_shared<demodulator::DemodulatorDove>(demodulator::DemodulatorDove::kInitialise);
			demod_dove->symbol_rate_hz = symbolrate_hz; demod_dove->carrier_hz = carrier_hz;
			front_iter->PostDove(demod_dove);
		}
	}
	
}

