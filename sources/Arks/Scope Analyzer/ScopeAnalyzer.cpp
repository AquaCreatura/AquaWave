#include "ScopeAnalyzer.h"
#include "special_defs/file_souce_defs.h"
#include "special_defs/analyzer_defs.h"
#include "ShipBuilder.h"
#include <ippvm.h>


#include <qmessagebox.h>

#include "Elements/DPX Spectrum/SpectrumDPX.h"
#include "Elements/Static SPG/Spectrogram.h"
#include "Elements/Constellation/Constellation.h"

using namespace fluctus;
using namespace scope_analyzer;
using namespace spectral_viewer;

ScopeAnalyzer::ScopeAnalyzer()
{
    window_ = new ScopeAnalyzerWindow;

	//Определяем графики гармонического анализа
	{
		charts_[kAcf]			 = std::make_shared<dpx_core::SpectrumDpx>(dpx_core::kDpxChartType::kACF);
		charts_[kAcf].need_resampler = false;

		charts_[kBandwidth]		 = std::make_shared<dpx_core::SpectrumDpx>(dpx_core::kDpxChartType::kFFT);
		charts_[kBandwidth].need_resampler = true;

		charts_[kConstellation] = std::make_shared<constel::Constellation>();
		charts_[kConstellation].need_resampler = false;


		charts_[kPhasorSpectrum]	= std::make_shared<dpx_core::SpectrumDpx>(dpx_core::kDpxChartType::kPhasor);
		charts_[kEnvelopeSpectrum]	= std::make_shared<dpx_core::SpectrumDpx>(dpx_core::kDpxChartType::kEnvelope);
		charts_[kPowerSpectrum]		= std::make_shared<dpx_core::SpectrumDpx>(dpx_core::kDpxChartType::kPower4x);
		
		for (auto chart_type : { kAcf, kPowerSpectrum , kPhasorSpectrum, kBandwidth, kEnvelopeSpectrum, kConstellation }) {
			auto chart_window = ShipBuilder::GetWindow(charts_[chart_type]);
			window_->AddChartWindow(chart_window, chart_type);
		}
	}
	window_->ActivateWindow(kPowerSpectrum);
	resampler_.SetPrecision(0);
	connect(window_, &ScopeAnalyzerWindow::FftChangeNeed, this, &ScopeAnalyzer::SetNewFftOrder);
}

ScopeAnalyzer::~ScopeAnalyzer()
{
	
}

bool ScopeAnalyzer::SendData(fluctus::DataInfo const & passed_unit)
{
	if (passed_unit.data_vec.size() != n_fft_ * sizeof(Ipp32fc))
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
		const int count_of_parts = resampled_buff_.size() / n_fft_;
		for (int part_counter = 0; part_counter < count_of_parts; part_counter++) {
			resampled.assign(resampled_buff_.data() + n_fft_ * part_counter, resampled_buff_.data() + n_fft_ * (part_counter + 1));
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
bool ScopeAnalyzer::SendDove(fluctus::DoveSptr const & sent_dove)
{
    // Если сообщение недействительно, выбрасываем исключение.
    if (!sent_dove) throw std::invalid_argument("Not created message sent!");
    
    // Получаем целевое значение и "мысль" из сообщения.
    auto target_val = sent_dove->target_ark;
    auto base_thought = sent_dove->base_thought;
    
    // Если "мысль" - запрос на диалог.
    if (base_thought & fluctus::DoveParrent::DoveThought::kGetDialog)
    {
        // Прикрепляем отрисовщик спектра к виджету сообщения.
        sent_dove->show_widget = window_;
        return true; // Запрос обработан.
    }
    if(base_thought == fluctus::DoveParrent::DoveThought::kTieSource)
    {
        if( target_val->GetArkType() == ArkType::kFileSource) 
			source_info_.ark = target_val;
        Reload();
    }
    //
    if(base_thought == fluctus::DoveParrent::DoveThought::kReset)
    {
        return Reload();
    }
	if (base_thought & fluctus::DoveParrent::DoveThought::kGetDescription)
	{
		sent_dove->description = selection_descr_;
		for (auto chart_iter : charts_) {
			if ((chart_iter.second == sent_dove->sender) && (chart_iter.second.need_resampler)) {
				sent_dove->description->samplerate_hz = resampled_samplerate_;
			}
		}
		
	}
	if (base_thought == fluctus::DoveParrent::DoveThought::kSpecialThought) {
		const auto special_thought = sent_dove->special_thought;
		if (auto spectral_dove = std::dynamic_pointer_cast<analyzer::AnalyzeDove>(sent_dove)) {

			if (special_thought & analyzer::AnalyzeDove::kStartFromFileSource) {
				return Restart(spectral_dove->freq_bounds_hz, spectral_dove->file_bounds_ratio);
			}
		};
		if (auto file_src_dove = std::dynamic_pointer_cast<file_source::FileSrcDove>(sent_dove)) {

		}

	}
    // Передаём сообщение базовому классу для дальнейшей обработки.
    return ArkBase::SendDove(sent_dove);
}

ArkType ScopeAnalyzer::GetArkType() const
{
    return ArkType::kScopeAnalyser;
}

bool ScopeAnalyzer::Reload()
{
	auto file_src = source_info_.ark.lock();

	if (!file_src) return true;	
	{
		auto req_dove = std::make_shared<fluctus::DoveParrent>(fluctus::DoveParrent::kGetDescription);
		if (!file_src->SendDove(req_dove) || !req_dove->description) {
			return false;
		}
		source_info_.descr = *req_dove->description;
	}
	return true;
}

bool ScopeAnalyzer::Restart(Limits<double> freq_bounds_Mhz, Limits<double> time_bounds)
{
	time_bounds_ = time_bounds;
	if (time_bounds_.delta() == 0) return false;
	auto arks = GetBehindArks();
	if (arks.empty()) return false;

	selection_descr_.bw_ratio_ = source_info_.descr.bw_ratio_;
	selection_descr_.carrier_hz = freq_bounds_Mhz.mid() * 1.e6;


	{
		auto file_src_ = arks.front();
		auto req_dove = std::make_shared<file_source::FileSrcDove>();
		req_dove->special_thought = file_source::FileSrcDove::kInitReaderInfo;
		req_dove->target_ark = shared_from_this();

		req_dove->time_bounds = time_bounds_;

		req_dove->setup.emplace();
		auto &setup = req_dove->setup;
		setup->carrier_hz = selection_descr_.carrier_hz;
		setup->chunk_size = n_fft_;		
		setup->banwidth_hz	 = freq_bounds_Mhz.delta() * 1.e6;
		setup->samplerate_hz = setup->banwidth_hz / source_info_.descr.bw_ratio_;

		if (!file_src_->SendDove(req_dove))
		{
			QMessageBox::warning( nullptr, "Cannot Send Data", "Do something with DPX or file source, or...");
			return false;
		}
		selection_descr_.samplerate_hz = setup->samplerate_hz; //Выставляем ЧД
		resampled_samplerate_ = setup->banwidth_hz * 2;
		resampler_.Init(setup->samplerate_hz, resampled_samplerate_, setup->samplerate_hz);
		resampled_unit_.freq_info_.samplerate_hz = resampled_samplerate_;
	}
	selection_descr_.count_of_samples = time_bounds.delta() * source_info_.descr.count_of_samples *
											selection_descr_.samplerate_hz / source_info_.descr.samplerate_hz;

	if (GetFrontArks().empty()) {
		for (auto chart_iter : charts_)
			ShipBuilder::Bind_SrcSink(shared_from_this(), chart_iter.second);
	}
	

	const int max_order = std::min(log2(selection_descr_.count_of_samples), 21.);

	auto req_dove = std::make_shared<spectral_viewer::SpectralDove>();
	req_dove->base_thought = DoveParrent::kReset;
	for (auto chart_iter : charts_)
		chart_iter.second->SendDove(req_dove);

	if(max_order > 0)
		window_->SetMaxFFtOrder(max_order);

	
	return true;
}

void scope_analyzer::ScopeAnalyzer::SetNewFftOrder(int need_order)
{
	auto arks = GetBehindArks();
	if (arks.empty()) return;

	n_fft_ = 1 << need_order;
	{
		auto req_dove = std::make_shared<spectral_viewer::SpectralDove>();
		req_dove->base_thought = DoveParrent::kSpecialThought;
		req_dove->special_thought = spectral_viewer::SpectralDove::kSetFFtOrder;
		req_dove->fft_order_ = log2(n_fft_);
		for (auto chart_iter : charts_) 
			chart_iter.second->SendDove(req_dove);

	}
	if (time_bounds_.delta() == 0) return;
	auto file_src_ = arks.front();
	auto req_dove = std::make_shared<file_source::FileSrcDove>();
	req_dove->special_thought = file_source::FileSrcDove::kInitReaderInfo | file_source::FileSrcDove::kAskLoopInRange;
	req_dove->target_ark = shared_from_this();

	req_dove->time_bounds = time_bounds_;
	
	req_dove->setup.emplace();
	auto &setup = req_dove->setup;
	setup->carrier_hz		= selection_descr_.carrier_hz;
	setup->samplerate_hz	= selection_descr_.samplerate_hz;
	setup->chunk_size	    = n_fft_;
	setup->banwidth_hz		= setup->samplerate_hz * source_info_.descr.bw_ratio_;
	if (!file_src_->SendDove(req_dove))
	{
		QMessageBox::warning(nullptr, "Cannot Send Data", "Do something with DPX or file source, or...");
		return;
	}


	return;
}

