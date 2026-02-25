#include "ScopeAnalyzer.h"
#include "special_defs/file_souce_defs.h"
#include "special_defs/analyzer_defs.h"
#include "ShipBuilder.h"
#include <ippvm.h>


#include <qmessagebox.h>

#include "Elements/DPX Spectrum/SpectrumDPX.h"
#include "Elements/Static SPG/Spectrogram.h"

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

		charts_[kPhasorSpectrum]	= std::make_shared<dpx_core::SpectrumDpx>(dpx_core::kDpxChartType::kFFT);
		charts_[kEnvelopeSpectrum]	= std::make_shared<dpx_core::SpectrumDpx>(dpx_core::kDpxChartType::kFFT);
		charts_[kPowerSpectrum]		= std::make_shared<dpx_core::SpectrumDpx>(dpx_core::kDpxChartType::kFFT);

		for (auto chart_type : { kAcf, kPowerSpectrum , kPhasorSpectrum, kBandwidth, kEnvelopeSpectrum}) {
			auto chart_window = ShipBuilder::GetWindow(charts_[chart_type]);
			window_->AddChartWindow(chart_window, chart_type);
		}
	}
	window_->ActivateWindow(kPowerSpectrum);


}

ScopeAnalyzer::~ScopeAnalyzer()
{
	
}

bool ScopeAnalyzer::SendData(fluctus::DataInfo const & data_info)
{
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
	auto arks = GetBehindArks();
	if (arks.empty()) return false;


	selection_descr_.carrier_hz = freq_bounds_Mhz.mid() * 1.e6;
	selection_descr_.samplerate_hz = freq_bounds_Mhz.delta() * 1.e6;
	selection_descr_.count_of_samples = time_bounds.delta() * source_info_.descr.count_of_samples *
											selection_descr_.samplerate_hz / source_info_.descr.samplerate_hz;
	
	int need_order = qBound(5l, std::lround(log2(selection_descr_.count_of_samples)/2), 12l);
	n_fft_ = 1 << need_order;
	{
		auto req_dove = std::make_shared<spectral_viewer::SpectralDove>();
		req_dove->base_thought = DoveParrent::kReset | DoveParrent::kSpecialThought;
		req_dove->special_thought = spectral_viewer::SpectralDove::kSetFFtOrder;
		req_dove->fft_order_ = log2(n_fft_);
		for (auto chart_iter : charts_) chart_iter.second->SendDove(req_dove);
		
	}




	auto file_src_ = arks.front();
	auto req_dove = std::make_shared<file_source::FileSrcDove>();
	req_dove->special_thought = file_source::FileSrcDove::kInitReaderInfo | file_source::FileSrcDove::kAskChunksInRange;
	req_dove->target_ark = shared_from_this();
	
	req_dove->time_point_start	= time_bounds.low;
	req_dove->time_point_end	= time_bounds.high;

	req_dove->carrier_hz		= selection_descr_.carrier_hz;
	req_dove->samplerate_hz		= selection_descr_.samplerate_hz;
	req_dove->data_size = n_fft_;
	if (!file_src_->SendDove(req_dove))
	{
		QMessageBox::warning(
			nullptr,                        // родительское окно (может быть this)
			"Cannot Send Data",            // заголовок окна
			"Do something with DPX or file source, or..."  // сообщение
		);
	}


	return true;
}

void ScopeAnalyzer::RequestSelectedData()
{
    auto arks = GetBehindArks();
    if(arks.empty()) return;

    auto file_src_ = arks.front();
    auto req_dove = std::make_shared<file_source::FileSrcDove>();
    req_dove->special_thought   = file_source::FileSrcDove::kInitReaderInfo |  file_source::FileSrcDove::kAskChunksInRange;
    req_dove->target_ark        = shared_from_this();
    req_dove->time_point_start  = 0;
	req_dove->time_point_end	= 1.;
    req_dove->data_size         = n_fft_;
    if (!file_src_->SendDove(req_dove))
    {
        QMessageBox::warning(
                            nullptr,                        // родительское окно (может быть this)
                            "Cannot Send Data",            // заголовок окна
                            "Do something with DPX or file source, or..."  // сообщение
                        );
    }

}
