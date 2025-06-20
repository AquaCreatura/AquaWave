#include "Spectrogram.h"
#include <ippvm.h>
using namespace spg_core; // Используем пространство имён dpx_core

// Конструктор: Инициализирует компонент для отрисовки спектра.
// parrent: Указатель на родительский QWidget.
Spectrogram::Spectrogram(QWidget * parrent)
{
    // Создаём и сохраняем умный указатель на объект DpxChart для отрисовки.
    dpx_drawer_ = std::make_shared<ChartSPG>(parrent);
}

// Отправляет данные для обработки спектра и отображения.
// data_info: Структура с входными данными и информацией о частоте.
bool Spectrogram::SendData(fluctus::DataInfo const & data_info)
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
    dpx_drawer_->PushData(power_vec, freq_bounds);
    
    return true; // Успех.
}

// Обрабатывает сообщения "Dove".
// sent_dove: Умный указатель на сообщение Dove.
bool Spectrogram::SendDove(fluctus::DoveSptr const & sent_dove)
{
    // Если сообщение недействительно, выбрасываем исключение.
    if (!sent_dove) throw std::invalid_argument("Not created message sent!");
    
    // Получаем целевое значение и "мысль" из сообщения.
    auto target_val = sent_dove->target_ark;
    auto base_thought = sent_dove->base_thought;
    
    // Если "мысль" - "ничего не делать", выходим.
    if      (base_thought == fluctus::DoveParrent::DoveThought::kNothing)
        return true;
    // Если "мысль" - запрос на диалог.
    else if (base_thought & fluctus::DoveParrent::DoveThought::kGetDialog)
    {
        // Прикрепляем отрисовщик спектра к виджету сообщения.
        sent_dove->show_widget = dpx_drawer_;
        return true; // Запрос обработан.
    }
    // Передаём сообщение базовому классу для дальнейшей обработки.
    return ArkBase::SendDove(sent_dove);
}