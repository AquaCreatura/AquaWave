#include "SpectralViewer.h"
#include "special_defs/file_souce_defs.h"
#include "special_defs/analyzer_defs.h"
#include "Arks/ShipBuilder.h"
#include <qmessagebox.h>

using namespace spectral_viewer;
using namespace aqua_gui;
// Конструктор: Инициализирует компонент для отрисовки спектра.
// parrent: Указатель на родительский QWidget.
SpectralViewer::SpectralViewer()
{
	selection_holder_ = std::make_shared<aqua_gui::SelectionHolder>();
    window_ = new SpectralViewerWindow;
	{
		spectrum_ = std::make_shared<dpx_core::SpectrumDpx>(dpx_core::kDpxChartType::kFFT); // Создание компонента для спектрального графика.
		spg_ = std::make_shared<spg_core::StaticSpg>(); // Создание компонента для спектрограммы.

		auto req_dove = std::make_shared<SpectralDove>(SpectralDove::kSetSelectionHolder);
		req_dove->sel_holder = selection_holder_;
		spectrum_->SendDove(req_dove);
		spg_->SendDove(req_dove);

		auto dpx_window = ShipBuilder::GetWindow(spectrum_);
		auto spg_window = ShipBuilder::GetWindow(spg_);

		window_->SetDpxSpectrumWindow(dpx_window);
		window_->SetSpectrogramWindow(spg_window);


		if (QPointer<ChartInterface> derivedPtr = qobject_cast<ChartInterface*>(dpx_window.data()) ) {
			connect(derivedPtr, &ChartInterface::SelectionIsReady, this, &SpectralViewer::OnSelectionIsReady);
		}
		if (QPointer<ChartInterface> derivedPtr = qobject_cast<ChartInterface*>(spg_window.data())) {
			connect(derivedPtr, &ChartInterface::SelectionIsReady, this, &SpectralViewer::OnSelectionIsReady);
		}
	}
	connect(window_, &SpectralViewerWindow::FftChangeNeed, this, &SpectralViewer::SetNewFftOrder);
}

SpectralViewer::~SpectralViewer()
{
	
}


// Отправляет данные для обработки спектра и отображения.
// data_info: Структура с входными данными и информацией о частоте.
bool SpectralViewer::SendData(fluctus::DataInfo const & data_info)
{
    // Если входные данные пусты, выходим.
    if(data_info.data_vec.empty()) return true;
	//spg_->SendData	(data_info);
	spectrum_->SendData	(data_info);
    return true; // Успех.
}

// Обрабатывает сообщения "Dove".
// sent_dove: Умный указатель на сообщение Dove.
bool SpectralViewer::SendDove(fluctus::DoveSptr const & sent_dove)
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
        if(target_val->GetArkType() != ArkType::kFileSource) throw std::logic_error("Only signal sources are able to connect!");
        src_info_.ark = target_val;
		//Определяем командное соединение
		{
			fluctus::DoveSptr req_dove = std::make_shared<fluctus::DoveParrent>(fluctus::DoveParrent::kTieSource);
			req_dove->target_ark = target_val;
			spg_->SendDove(req_dove);
			spectrum_->SendDove(req_dove);
		}
    }
    //
    if(base_thought == fluctus::DoveParrent::DoveThought::kReset)
    {
        return Reload();
    }

    // Передаём сообщение базовому классу для дальнейшей обработки.
    return ArkBase::SendDove(sent_dove);
}




ArkType SpectralViewer::GetArkType() const
{
    return ArkType::kSpectralViewer;
}

bool SpectralViewer::Reload()
{
    auto file_src = src_info_.ark.lock();
	
    if(!file_src) return true;

	
	auto parrent_dove = std::make_shared<fluctus::DoveParrent>(fluctus::DoveParrent::kGetDescription);
	if (!file_src->SendDove(parrent_dove) || !parrent_dove->description) {
		return false;
	}
	src_info_.descr = *parrent_dove->description;
	const int max_order = std::min(log2(parrent_dove->description->count_of_samples), 21.);
	window_->SetMaxFFtOrder(max_order);
	
	parrent_dove->base_thought = fluctus::DoveParrent::DoveThought::kReset;
	spg_->SendDove(parrent_dove);
	spectrum_->SendDove(parrent_dove);

	{
		using namespace file_source;
		auto req_dove = std::make_shared<FileSrcDove>(FileSrcDove::kInitiate | FileSrcDove::kAskLoopInRange);
		req_dove->target_ark = shared_from_this();
		req_dove->time_bounds = { 0., 1. };
		req_dove->setup.emplace();
		req_dove->setup->chunk_size = n_fft_;
		req_dove->setup->carrier_hz = src_info_.descr.carrier_hz;
		req_dove->setup->banwidth_hz = src_info_.descr.samplerate_hz * src_info_.descr.bw_ratio_;
		req_dove->setup->samplerate_hz = src_info_.descr.samplerate_hz;
		if (!file_src->SendDove(req_dove)) {
			QMessageBox::warning(nullptr, "Cannot Send Data", "Do something with DPX or file source, or...");
		}
	}
    return true;
}

void SpectralViewer::SetNewFftOrder(int n_fft_order)
{
	int new_fft = 1 << n_fft_order;
	if (n_fft_ == new_fft) return;
	n_fft_ = new_fft;
	auto file_src = src_info_.ark.lock();
	if (!file_src) return;

	auto spectral_dove = std::make_shared<SpectralDove>(SpectralDove::SpectralThought::kSetFFtOrder);
	spectral_dove->fft_order_ = n_fft_order;
	spg_->SendDove(spectral_dove);
	spectrum_->SendDove(spectral_dove);

	

	auto file_src_dove = std::make_shared<file_source::FileSrcDove>(file_source::FileSrcDove::kSetChunkSize);
	file_src_dove->target_ark = shared_from_this();
	file_src_dove->time_bounds = { 0., 1. };
	file_src_dove->setup.emplace()->chunk_size = n_fft_;
	if (!file_src->SendDove(file_src_dove)) {
		QMessageBox::warning(nullptr, "Cannot Send Data", "Do something with DPX or file source, or..."  );
	}
}

void spectral_viewer::SpectralViewer::OnSelectionIsReady()
{
	auto front_arks = GetFrontArks();
	for (auto front_iter : front_arks) {
		if (front_iter->GetArkType() == fluctus::kScopeAnalyser) {
			auto analyze_dove = std::make_shared<analyzer::AnalyzeDove>(analyzer::AnalyzeDove::kStartFromFileSource);
			auto cur_sel = selection_holder_->GetCurrentSelection();
			if (cur_sel.freq_bounds.delta() < 0) std::swap(cur_sel.freq_bounds.low, cur_sel.freq_bounds.high);
			if (cur_sel.time_bounds.delta() < 0) std::swap(cur_sel.time_bounds.low, cur_sel.time_bounds.high);
			analyze_dove->freq_bounds_hz	= cur_sel.freq_bounds;
			analyze_dove->file_bounds_ratio = cur_sel.time_bounds;
			front_iter->SendDove(analyze_dove);
		}
	}
}


