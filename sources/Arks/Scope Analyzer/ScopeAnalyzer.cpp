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

	//Определяем спектрограмму и спектр выделенной полосы
	{
		charts_[kBaseSpectrum] = std::make_shared<dpx_core::SpectrumDpx>(dpx_core::kDpxChartType::kFFT);
		charts_[kBaseSpg] = std::make_shared<spg_core::StaticSpg>();

		auto req_dove = std::make_shared<SpectralDove>(SpectralDove::kSetSelectionHolder);
		req_dove->sel_holder = std::make_shared<aqua_gui::SelectionHolder>();

		for (auto chart_type : { kBaseSpectrum, kBaseSpg }) {
			auto chart_window = ShipBuilder::GetWindow(charts_[chart_type]);
			window_->AddChartWindow(chart_window, chart_type);
			charts_[chart_type]->SendDove(req_dove);
		}
	}
	//Определяем графики гармонического анализа
	{
		charts_[kAcf]			 = std::make_shared<dpx_core::SpectrumDpx>(dpx_core::kDpxChartType::kACF);
		charts_[kBandwidth]		 = std::make_shared<dpx_core::SpectrumDpx>(dpx_core::kDpxChartType::kFFT);

		charts_[kPhasorSpectrum]	= std::make_shared<dpx_core::SpectrumDpx>(dpx_core::kDpxChartType::kPhasor);
		charts_[kEnvelopeSpectrum]	= std::make_shared<dpx_core::SpectrumDpx>(dpx_core::kDpxChartType::kEnvelope);
		charts_[kPowerSpectrum]		= std::make_shared<dpx_core::SpectrumDpx>(dpx_core::kDpxChartType::kPower4x);
		charts_[kConstellation]		= std::make_shared<constel::Constellation>();
		for (auto chart_type : { kAcf, kPowerSpectrum , kPhasorSpectrum, kBandwidth, kEnvelopeSpectrum, kConstellation }) {
			auto chart_window = ShipBuilder::GetWindow(charts_[chart_type]);
			window_->AddChartWindow(chart_window, chart_type);
		}
	}
	window_->ActivateWindow(kPowerSpectrum);

	connect(window_, &ScopeAnalyzerWindow::FftChangeNeed, this, &ScopeAnalyzer::SetNewFftOrder);
}

ScopeAnalyzer::~ScopeAnalyzer()
{
	
}

bool ScopeAnalyzer::SendData(fluctus::DataInfo const & data_info)
{
	if (data_info.data_vec.size() != n_fft_ * 8)
		return false;
	for (auto chart_iter : charts_) {
		chart_iter.second->SendData(data_info);
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
	if (GetFrontArks().empty()) {
		for (auto chart_iter : charts_) 
			ShipBuilder::Bind_SrcSink(shared_from_this(), chart_iter.second);	
	}
	time_bounds_ = time_bounds;
	auto arks = GetBehindArks();
	if (arks.empty()) return false;

	selection_descr_.bw_ratio_ = source_info_.descr.bw_ratio_;
	selection_descr_.carrier_hz = freq_bounds_Mhz.mid() * 1.e6;
	selection_descr_.samplerate_hz = freq_bounds_Mhz.delta() * 1.e6 / source_info_.descr.bw_ratio_;
	selection_descr_.count_of_samples = time_bounds.delta() * source_info_.descr.count_of_samples *
											selection_descr_.samplerate_hz / source_info_.descr.samplerate_hz;
	
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
		QMessageBox::warning(
			nullptr,                        // родительское окно (может быть this)
			"Cannot Send Data",            // заголовок окна
			"Do something with DPX or file source, or..."  // сообщение
		);
	}


	return;
}

