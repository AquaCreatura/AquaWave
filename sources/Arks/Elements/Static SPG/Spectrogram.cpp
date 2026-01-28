#include "Spectrogram.h"
#include <ippvm.h>
#include "special_defs/file_souce_defs.h"
#include "qmessagebox.h"
using namespace spg_core; // Используем пространство имён dpx_core

// Конструктор: Инициализирует компонент для отрисовки спектра.
// parrent: Указатель на родительский QWidget.
StaticSpg::StaticSpg(QWidget * parrent) : 
    spg_drawer_(new spg_core::ChartSPG()),
    requester_(spg_drawer_->GetSpectrogramInfo(), time_bounds_) 
{
    //window_->SetChartWindow(spg_drawer_);
	//connect(window_, &SpgWindow::FftChangeNeed, spg_drawer_, &ChartSPG::SetFftOrder);
	
}

spg_core::StaticSpg::~StaticSpg()
{
	printf_s("Destroyed...");
	requester_.StartProcess(false);
}

// Отправляет данные для обработки спектра и отображения.
// data_info: Структура с входными данными и информацией о частоте.
bool StaticSpg::SendData(fluctus::DataInfo const & data_info)
{
    // Если входные данные пусты, выходим.
    if(data_info.data_vec.empty()) return true;

    // Ссылки на информацию о частоте и входные комплексные данные.
    auto &freq_info  = data_info.freq_info_;
    auto &passed_data = (std::vector<Ipp32fc>&)data_info.data_vec; // Приведение типа.
    
    // Буфер для результата БПФ.
    std::vector<Ipp32fc> transformed_data(passed_data.size());
    // Выполняем прямое Быстрое Преобразование Фурье (БПФ).
    fft_worker_.EnableSwapOnForward(true); 
    if(!fft_worker_.ForwardFFT(passed_data, transformed_data))
        return false;

    // Буфер для магнитуд спектра (амплитуд).
    std::vector<Ipp32f> power_vec(transformed_data.size());
    
    // Вычисляем магнитуду (амплитуду) комплексных чисел.
    ippsPowerSpectr_32fc(transformed_data.data(), power_vec.data(), passed_data.size());
	ippsAddC_32f_I(0.0001, power_vec.data(), power_vec.size());
    ippsLog10_32f_A11(power_vec.data(), power_vec.data(), power_vec.size());
    ippsMulC_32f_I(10, power_vec.data(), power_vec.size());
    
    // Определяем границы частотного диапазона для отображения.
    Limits<double> freq_bounds = {freq_info.carrier - freq_info.samplerate / 2.,
                                   freq_info.carrier + freq_info.samplerate / 2.};
    
    // Отправляем вычисленные магнитуды и частотные границы в отрисовщик.
    draw_data draw_data;
    draw_data.freq_bounds = freq_bounds;
    draw_data.time_pos    = data_info.time_point;
    draw_data.data        = power_vec;
    spg_drawer_->PushData(draw_data);
    
    return true; // Успех.
}

// Обрабатывает сообщения "Dove".
// sent_dove: Умный указатель на сообщение Dove.
bool StaticSpg::SendDove(fluctus::DoveSptr const & sent_dove)
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
        sent_dove->show_widget = spg_drawer_;
        return true; // Запрос обработан.
    }
    // Передаём сообщение базовому классу для дальнейшей обработки.
    if(base_thought & DoveParrent::DoveThought::kTieSource)
    {
        if(target_val->GetArkType() != ArkType::kFileSource) throw std::logic_error("Only signal sources are able to connect!");
        src_info_.ark = target_val;
    }
    if (base_thought == fluctus::DoveParrent::DoveThought::kReset)
    {
        return Reload();
    }
	if (base_thought == fluctus::DoveParrent::DoveThought::kSpecialThought) {
		const auto special_thought = sent_dove->special_thought;
		if (auto spectral_dove = std::dynamic_pointer_cast<spectral_viewer::SpectralDove>(sent_dove)) {
			if (special_thought & spectral_viewer::SpectralDove::kSetFFtOrder) {
				spg_drawer_->SetFftOrder(*spectral_dove->fft_order_);
			}
			if (special_thought & spectral_viewer::SpectralDove::kSetSelectionHolder) {
				selection_holder_ = *spectral_dove->sel_holder;	
				spg_drawer_->SetSelectionHolder(selection_holder_);
			}
		};

	}
    return ArkBase::SendDove(sent_dove);
}

ArkType spg_core::StaticSpg::GetArkType() const
{
    return ArkType::kStaticSpg;
}

bool spg_core::StaticSpg::Reload()
{
    auto file_src = src_info_.ark.lock();
    if(!file_src) 
    {
        return true;
    }
    requester_.Initialise(file_src, this->shared_from_this());

    auto req_dove = std::make_shared<file_source::FileSrcDove>();
    req_dove->base_thought      = fluctus::DoveParrent::DoveThought::kSpecialThought;
    req_dove->special_thought   = file_source::FileSrcDove::kGetFileInfo;
    if (!file_src->SendDove(req_dove) || !req_dove->file_info) {
        return false;
    }
    src_info_.info.carrier      = req_dove->file_info->carrier_hz_;
    src_info_.info.samplerate   = req_dove->file_info->samplerate_hz_;
	Limits<double> new_hor_bounds = { 0., std::max(1.,double(req_dove->file_info->count_of_samples)) };
	if (1) {
		time_bounds_.source = new_hor_bounds;
		spg_drawer_->ClearData();
		spg_drawer_->SetHorizontalMinMaxBounds(new_hor_bounds);
		{
			Limits<double> bounds_hz = {
				double(src_info_.info.carrier) - src_info_.info.samplerate / 2.,
				double(src_info_.info.carrier) + src_info_.info.samplerate / 2.
			};
			freq_divider_ = 1.e6;

			bounds_hz = bounds_hz / freq_divider_;
			spg_drawer_->SetVerticalMinMaxBounds(bounds_hz);
			spg_drawer_->SetVerticalSuffix("MHz");
		}
	}
    
    return true;
}
