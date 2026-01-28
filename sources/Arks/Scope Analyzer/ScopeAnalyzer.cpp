#include "ScopeAnalyzer.h"
#include "special_defs/file_souce_defs.h"
#include "special_defs/analyzer_defs.h"
#include <ippvm.h>

#include <qmessagebox.h>

using namespace fluctus;

ScopeAnalyzer::ScopeAnalyzer()
{

    window_ = new ScopeAnalyzerWindow;
}

ScopeAnalyzer::~ScopeAnalyzer()
{
	
}

bool ScopeAnalyzer::SendData(fluctus::DataInfo const & data_info)
{
	return false;
}

// Отправляет данные для обработки спектра и отображения.
// data_info: Структура с входными данными и информацией о частоте.
bool SendData(fluctus::DataInfo const & data_info)
{

    return true; // Успех.
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
			src_info_.ark = target_val;
        Reload();
    }
    //
    if(base_thought == fluctus::DoveParrent::DoveThought::kReset)
    {
        return Reload();
    }
	if (base_thought == fluctus::DoveParrent::DoveThought::kSpecialThought) {
		const auto special_thought = sent_dove->special_thought;
		if (auto spectral_dove = std::dynamic_pointer_cast<analyzer::AnalyzeDove>(sent_dove)) {
			if (special_thought & analyzer::AnalyzeDove::kStartFromFileSource) {
				return Restart(spectral_dove->freq_bounds_hz, spectral_dove->file_bounds_ratio);
			}
		};
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
	auto file_src = src_info_.ark.lock();

	if (!file_src) return true;

	auto req_dove = std::make_shared<DoveParrent>();
	{
		auto req_dove = std::make_shared<file_source::FileSrcDove>();
		req_dove->base_thought = fluctus::DoveParrent::DoveThought::kSpecialThought;
		req_dove->special_thought = file_source::FileSrcDove::kGetFileInfo;
		if (!file_src->SendDove(req_dove) || !req_dove->file_info) {
			return false;
		}
		const int max_order = std::min(log2(req_dove->file_info->count_of_samples), 21.);
		//window_->SetMaxFFtOrder(max_order);
	}
	req_dove->base_thought = fluctus::DoveParrent::DoveThought::kReset;
	spg_->SendDove(req_dove);
	dpx_spectrum_->SendDove(req_dove);
	return true;
}

bool ScopeAnalyzer::Restart(Limits<double> freq_bounds_hz, Limits<double> time_bounds)
{

	return true;
}

void ScopeAnalyzer::SetNewFftOrder(int n_fft_order)
{
	auto file_src = src_info_.ark.lock();
	if (!file_src) return true;

	auto req_dove = std::make_shared<file_source::FileSrcDove>();
	req_dove->base_thought = fluctus::DoveParrent::DoveThought::kSpecialThought;
	req_dove->special_thought = file_source::FileSrcDove::kInitReaderInfo | file_source::FileSrcDove::kAskChunksInRange;
	req_dove->target_ark = shared_from_this();
	req_dove->time_point_start = 0;
	req_dove->time_point_end = 1.;
	req_dove->data_size = n_fft_;
	if (!file_src->SendDove(req_dove))
	{
		QMessageBox::warning(
			nullptr,                        // родительское окно (может быть this)
			"Cannot Send Data",            // заголовок окна
			"Do something with DPX or file source, or..."  // сообщение
		);


	}
}

void ScopeAnalyzer::RequestSelectedData()
{
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
