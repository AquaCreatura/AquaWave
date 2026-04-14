#include "SpectrumDPX.h" // Включаем заголовок класса SpectrumDpx
#include "special_defs/file_souce_defs.h"
#include <ippvm.h>

#include <qmessagebox.h>
using namespace dpx_core; // Используем пространство имён dpx_core
using namespace pipes; 
// Конструктор: Инициализирует компонент для отрисовки спектра.
// parrent: Указатель на родительский QWidget.
dpx_core::SpectrumDpx::SpectrumDpx(kDpxChartType chart_type)
{
	chart_type_ = chart_type;
	
	{
		const bool is_analyze_chart = (chart_type != kDpxChartType::kFFT);
		const auto chart_type = is_analyze_chart ? aqua_gui::ChartDomainType::kAnalyzeDomain : aqua_gui::ChartDomainType::kFreqDomain;
		dpx_drawer_ = new ChartDPX(nullptr, chart_type); // Создаём указатель на объект DpxChart для отрисовки.
	}
	
	switch (chart_type)
	{
	case dpx_core::kDpxChartType::kFFT:
		pipe_line_.AddNextPipe(std::make_shared<FFtPipe>());
		pipe_line_.AddNextPipe(std::make_shared<PowerToDbPipe>());
		break;
	case dpx_core::kDpxChartType::kACF: 
		pipe_line_.AddNextPipe(std::make_shared<AcfPipe>());
		break;	
	case dpx_core::kDpxChartType::kEnvelope: 
		pipe_line_.AddNextPipe(std::make_shared<EnvelopePipe>());
		pipe_line_.AddNextPipe(std::make_shared<FFtPipe>());
		pipe_line_.AddNextPipe(std::make_shared<PrecisedPartSaver>(2, 1));
		pipe_line_.AddNextPipe(std::make_shared<ZeroFirstSamples>(0.05)); //Обнуляем нулевую гармониу
		break;	
	case dpx_core::kDpxChartType::kPhasor: 
		pipe_line_.AddNextPipe(std::make_shared<SamplesDiffPipe>());
		pipe_line_.AddNextPipe(std::make_shared<PhasorPipe>());
		pipe_line_.AddNextPipe(std::make_shared<FFtPipe>());
		pipe_line_.AddNextPipe(std::make_shared<PrecisedPartSaver>(2,1)); 
		break;
	case dpx_core::kDpxChartType::kPower4x: 
		pipe_line_.AddNextPipe(std::make_shared<MulByItSelfPipe>()); //2 степень
		pipe_line_.AddNextPipe(std::make_shared<MulByItSelfPipe>()); //4 степень
		pipe_line_.AddNextPipe(std::make_shared<FFtPipe>());
		break;
	}
	dpx_drawer_->SetVerticalSuffix("db");
}

dpx_core::SpectrumDpx::~SpectrumDpx()
{
	printf_s("SpectrumDPX Destroyed...");
}

// Отправляет данные для обработки спектра и отображения.
// data_info: Структура с входными данными и информацией о частоте.
bool SpectrumDpx::SendData(fluctus::DataInfo const & data_info)
{
    if(data_info.data_vec.empty()) return true; // Если входные данные пусты, выходим.
    auto &freq_info  = data_info.freq_info_;
    auto &passed_data = (std::vector<Ipp32fc>&)data_info.data_vec; // Приведение типа.
	if (n_fft_ != passed_data.size()) return false;

	pipe_line_.Process(passed_data);
	draw_data draw_data;
	if (chart_type_ == kDpxChartType::kFFT) {
		Limits<double> freq_bounds = { freq_info.carrier_hz - freq_info.samplerate_hz / 2.,
									   freq_info.carrier_hz + freq_info.samplerate_hz / 2. };
		draw_data.freq_bounds = freq_bounds / freq_divider_;
	}
	else
	{
		draw_data.freq_bounds = {0,0};
	}
    
    draw_data.time_pos    = data_info.time_point;
	draw_data.data = pipe_line_.meta->float_data;
    dpx_drawer_->PushData(draw_data);
    
    return true; 
}

// Обрабатывает сообщения "Dove".
// sent_dove: Умный указатель на сообщение Dove.
bool dpx_core::SpectrumDpx::SendDove(fluctus::DoveSptr const & sent_dove)
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
        sent_dove->show_widget = dpx_drawer_;
        return true; // Запрос обработан.
    }
    if(base_thought & fluctus::DoveParrent::DoveThought::kTieSource)
    {
        src_info_.ark = target_val;
		Reload();
    }
    if(base_thought & fluctus::DoveParrent::DoveThought::kReset)
    {
		Reload();
		RequestSelectedData();
    }
	if (base_thought & fluctus::DoveParrent::DoveThought::kSpecialThought) {
		const auto special_thought = sent_dove->special_thought;
		if (auto spectral_dove = std::dynamic_pointer_cast<spectral_viewer::SpectralDove>(sent_dove)) {

			if (special_thought & spectral_viewer::SpectralDove::kSetFFtOrder) {
				SetNewFftOrder(*spectral_dove->fft_order_);
			}

			if (special_thought & spectral_viewer::SpectralDove::kSetSelectionHolder) {
				selection_holder_ = *spectral_dove->sel_holder;
				dpx_drawer_->SetSelectionHolder(selection_holder_);
			}
		};
	}
    // Передаём сообщение базовому классу для дальнейшей обработки.
    return ArkBase::SendDove(sent_dove);
}

ArkType dpx_core::SpectrumDpx::GetArkType() const
{
    return ArkType::kSpectrumDpx;
}

bool dpx_core::SpectrumDpx::Reload()
{
	dpx_drawer_->ClearData();

    auto file_src = src_info_.ark.lock();
    if(!file_src) return true;
    
	auto req_dove = std::make_shared<fluctus::DoveParrent>(fluctus::DoveParrent::kGetDescription);
	req_dove->sender = shared_from_this();
    if (!file_src->SendDove(req_dove) || !req_dove->description)
    {
        return false;
    }
    src_info_.descr = *req_dove->description;



	UpdateAxisBounds();
	
    return true;
}

void dpx_core::SpectrumDpx::SetNewFftOrder(int n_fft_order)
{
	n_fft_ = 1 << n_fft_order;
	UpdateAxisBounds();

	dpx_drawer_->ClearData();
	RequestSelectedData();
}

void dpx_core::SpectrumDpx::UpdateAxisBounds()
{
	const auto& d = src_info_.descr;

	const double SR = d.samplerate_hz;
	const double Fc = d.carrier_hz;

	Limits<double> bounds{};
	std::string suffix;
	double divider = 1.0;

	switch (chart_type_)
	{
	case dpx_core::kDpxChartType::kACF:
	{
		double duration_sec = n_fft_ / SR;
		bounds = { 0.0, duration_sec * 1e3 };
		suffix = "ms";
		break;
	}

	case dpx_core::kDpxChartType::kEnvelope:
	case dpx_core::kDpxChartType::kPhasor:
	{
		double max_freq_hz = SR / 2.0;
		bounds = { 0.0, max_freq_hz };
		divider = 1e3;
		suffix = "kHz";
		break;
	}

	case dpx_core::kDpxChartType::kPower4x:
	{
		double half_band_hz = (SR / 4.0) / 2;
		bounds = { Fc-half_band_hz, Fc + half_band_hz };
		divider = 1e3;
		suffix = "kHz";
		break;
	}

	default:
	{
		bounds = { Fc - SR / 2.0, Fc + SR / 2.0 };
		divider = 1e6;
		suffix = "MHz";
		break;
	}
	}

	freq_divider_ = divider;
	bounds = bounds / freq_divider_;

	dpx_drawer_->SetHorizontalMinMaxBounds(bounds);
	dpx_drawer_->SetHorizontalSuffix(suffix.c_str());
}
void SpectrumDpx::RequestSelectedData()
{
    auto arks = GetBehindArks();
    if(arks.empty()) return;
    auto file_src_ = arks.front();
    auto req_dove = std::make_shared<file_source::FileSrcDove>();
    req_dove->base_thought      = fluctus::DoveParrent::DoveThought::kSpecialThought;
    req_dove->special_thought   = file_source::FileSrcDove::kInitReaderInfo |  file_source::FileSrcDove::kAskLoopInRange;
    req_dove->target_ark        = shared_from_this();
	req_dove->time_bounds		= { 0., 1. };
	auto &setup = req_dove->setup;
	setup.emplace();
	setup->chunk_size = n_fft_;
	setup->carrier_hz = src_info_.descr.carrier_hz;
	setup->samplerate_hz = src_info_.descr.samplerate_hz;
	setup->banwidth_hz = setup->samplerate_hz * src_info_.descr.bw_ratio_;
    if (!file_src_->SendDove(req_dove))
    {
        QMessageBox::warning(
                            nullptr,                        // родительское окно (может быть this)
                            "Cannot Send Data",            // заголовок окна
                            "Do something with DPX or file source, or..."  // сообщение
                        );
    }

}
