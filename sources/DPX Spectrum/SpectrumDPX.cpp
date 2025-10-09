#include "SpectrumDPX.h" // Включаем заголовок класса SpectrumDPX
#include "File Source/file_souce_defs.h"
#include <ippvm.h>

#include <qmessagebox.h>
using namespace dpx_core; // Используем пространство имён dpx_core

// Конструктор: Инициализирует компонент для отрисовки спектра.
// parrent: Указатель на родительский QWidget.
dpx_core::SpectrumDPX::SpectrumDPX()
{
    dpx_drawer_ = new ChartDPX();     // Создаём указатель на объект DpxChart для отрисовки.
    window_ = new DpxWindow;
    window_->SetChartWindow(dpx_drawer_);
	dpx_drawer_->SetVerticalSuffix("db");
    connect(window_, &DpxWindow::NeedDoSomething, this, &SpectrumDPX::RequestSelectedData);
}

dpx_core::SpectrumDPX::~SpectrumDPX()
{
	
}

// Отправляет данные для обработки спектра и отображения.
// data_info: Структура с входными данными и информацией о частоте.
bool SpectrumDPX::SendData(fluctus::DataInfo const & data_info)
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
    ippsLog10_32f_A11(power_vec.data(), power_vec.data(), power_vec.size());
    ippsMulC_32f_I(10, power_vec.data(), power_vec.size());
    
    // Определяем границы частотного диапазона для отображения.
    Limits<double> freq_bounds = {freq_info.carrier - freq_info.samplerate / 2.,
                                   freq_info.carrier + freq_info.samplerate / 2.};
    
    // Отправляем вычисленные магнитуды и частотные границы в отрисовщик.
    draw_data draw_data;
    draw_data.freq_bounds = freq_bounds / freq_divider_;
    draw_data.time_pos    = data_info.time_point;
    draw_data.data        = power_vec;
    dpx_drawer_->PushData(draw_data);
    
    return true; // Успех.
}

// Обрабатывает сообщения "Dove".
// sent_dove: Умный указатель на сообщение Dove.
bool dpx_core::SpectrumDPX::SendDove(fluctus::DoveSptr const & sent_dove)
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
    if(base_thought == fluctus::DoveParrent::DoveThought::kTieBehind)
    {
        if(target_val->GetArkType() != ArkType::kFileSource) throw std::logic_error("Only signal sources are able to connect!");
        src_info_.ark = target_val;
        Reload();
    }
    //
    if(base_thought == fluctus::DoveParrent::DoveThought::kReset)
    {
        return Reload();
    }

    // Передаём сообщение базовому классу для дальнейшей обработки.
    return ArkBase::SendDove(sent_dove);
}

ArkType dpx_core::SpectrumDPX::GetArkType() const
{
    return ArkType::kSpectrumDpx;
}

bool dpx_core::SpectrumDPX::Reload()
{
    auto file_src = src_info_.ark.lock();
	dpx_drawer_->ClearData();
    if(!file_src) return true;
    
    
    auto req_dove = std::make_shared<file_source::FileSrcDove>();
    req_dove->base_thought      = fluctus::DoveParrent::DoveThought::kSpecialThought;
    req_dove->special_thought   = file_source::FileSrcDove::kGetFileInfo;
    if (!file_src->SendDove(req_dove) || !req_dove->file_info)
    {
        QMessageBox::warning( nullptr, "Cannot Get info", "Do something with DPX or file source, or..." );
        return false;
    }
    src_info_.info.carrier      = (*req_dove->file_info).carrier_hz_;
    src_info_.info.samplerate   = (*req_dove->file_info).samplerate_hz_;
	Limits<double> bounds_hz = {
		double(src_info_.info.carrier) - src_info_.info.samplerate / 2.,
		double(src_info_.info.carrier) + src_info_.info.samplerate / 2.
	};
	freq_divider_ = 1.e6;

	bounds_hz = bounds_hz / freq_divider_;
	dpx_drawer_->SetHorizontalMinMaxBounds(bounds_hz);
	dpx_drawer_->SetHorizontalSuffix("MHz");

    RequestSelectedData();
    
    return true;
}

void SpectrumDPX::RequestSelectedData()
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
    req_dove->data_size         = 1'024 * 16;
    if (!file_src_->SendDove(req_dove))
    {
        QMessageBox::warning(
                            nullptr,                        // родительское окно (может быть this)
                            "Cannot Send Data",            // заголовок окна
                            "Do something with DPX or file source, or..."  // сообщение
                        );
    }

}
