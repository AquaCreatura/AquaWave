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
		dpx_spectrum_ = std::make_shared<dpx_core::SpectrumDPX>(); // Создание компонента для спектрального графика.
		spectrogram_ = std::make_shared<spg_core::Spectrogram>(); // Создание компонента для спектрограммы.

		std::shared_ptr<spectral_viewer::SpectralDove> req_dove = std::make_shared<spectral_viewer::SpectralDove>();
		req_dove->special_thought = spectral_viewer::SpectralDove::kSetSelectionHolder;		
		req_dove->sel_holder = selection_holder_;
		dpx_spectrum_->SendDove(req_dove);
		spectrogram_->SendDove(req_dove);

		auto dpx_window = ShipBuilder::GetWindow(dpx_spectrum_);
		auto spg_window = ShipBuilder::GetWindow(spectrogram_);

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
	//spectrogram_->SendData	(data_info);
	dpx_spectrum_->SendData	(data_info);
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
			fluctus::DoveSptr req_dove = std::make_shared<fluctus::DoveParrent>();
			req_dove->base_thought = fluctus::DoveParrent::kTieSource; // Изменение типа запроса: привязать.
			req_dove->target_ark = target_val;
			spectrogram_->SendDove(req_dove);
			dpx_spectrum_->SendDove(req_dove);
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
	
    auto req_dove = std::make_shared<DoveParrent>();
	{
		auto req_dove = std::make_shared<file_source::FileSrcDove>();
		req_dove->base_thought = fluctus::DoveParrent::DoveThought::kSpecialThought;
		req_dove->special_thought = file_source::FileSrcDove::kGetFileInfo;
		if (!file_src->SendDove(req_dove) || !req_dove->file_info) {
			return false;
		}
		const int max_order = std::min(log2(req_dove->file_info->count_of_samples), 21.);
		window_->SetMaxFFtOrder(max_order);
	}
	req_dove->base_thought = fluctus::DoveParrent::DoveThought::kReset;
	spectrogram_->SendDove(req_dove);
	dpx_spectrum_->SendDove(req_dove);
	RequestSelectedData();
    return true;
}

void SpectralViewer::SetNewFftOrder(int n_fft_order)
{
	int new_fft = 1 << n_fft_order;
	if (n_fft_ == new_fft) return;
	n_fft_ = new_fft;
	auto req_dove = std::make_shared<SpectralDove>();
	req_dove->special_thought = SpectralDove::SpectralThought::kSetFFtOrder;
	req_dove->fft_order_ = n_fft_order;
	spectrogram_->SendDove(req_dove);
	dpx_spectrum_->SendDove(req_dove);
	RequestSelectedData();
}

void spectral_viewer::SpectralViewer::OnSelectionIsReady()
{
	auto front_arks = GetFrontArks();
	for (auto front_iter : front_arks) {
		if (front_iter->GetArkType() == fluctus::kScopeAnalyser) {
			auto an_dove = std::make_shared<analyzer::AnalyzeDove>();
			an_dove->special_thought = analyzer::AnalyzeDove::kStartFromFileSource;
			front_iter->SendDove(an_dove);
		}
	}
}

void SpectralViewer::RequestSelectedData()
{
	return;
    auto arks = GetBehindArks();
    if(arks.empty()) return;
    auto file_src_ = arks.front();
    auto req_dove = std::make_shared<file_source::FileSrcDove>();
    req_dove->base_thought      = fluctus::DoveParrent::DoveThought::kSpecialThought;
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
