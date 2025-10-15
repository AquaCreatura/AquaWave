#include "PowerManager.h"
#include <ipp.h>
#include <algorithm>

using namespace aqua_gui;

PowerLimitMan::PowerLimitMan()
{
	need_reset_bounds_ = true;
    is_adaptive_mode_ = true;              // Адаптивный режим включён по умолчанию
	power_bounds_ = { 0., 1. };
    power_margins_ = {-0.15, 0.15};         // Запас (margin) на расширение пределов мощности
}

void PowerLimitMan::SetNewViewBounds(const Limits<double>& x_bounds) {
    view_bounds_ = x_bounds;               // Установка новых границ видимой области
}

void PowerLimitMan::SetPowerBounds(const Limits<double>& power_bounds) {
	need_reset_bounds_ = false;
    power_bounds_ = power_bounds;          // Установка границ мощности вручную
}

void aqua_gui::PowerLimitMan::SetPowerMargins(const Limits<double>& power_margins)
{
    power_margins_ = power_margins;        // Установка долей для расширения границ мощности
}

Limits<double> PowerLimitMan::GetPowerBounds() const {
    return power_bounds_.load();           // Получение текущих границ мощности
}

void PowerLimitMan::UpdateBounds(const std::vector<float>& data, const Limits<double>& data_bounds) {
    if (!IsAdaptiveMode() || data.empty()) {
        return;                            // Выход, если режим не адаптивный или данные пусты
    }

    // Пересечение видимых границ и границ данных
    Limits<double> view = view_bounds_.load();
    double intersect_low = std::max(data_bounds.low, view.low);
    double intersect_high = std::min(data_bounds.high, view.high);
    
    if (intersect_low >= intersect_high) {
        return;                            // Нет пересечения
    }

    // Вычисление индексов вектора данных, соответствующих границам пересечения
    double data_range = data_bounds.delta();
    if (data_range <= 0.0) {
        return;                            // Некорректный диапазон данных
    }

    size_t data_size = data.size();
    size_t start_idx = static_cast<size_t>((intersect_low - data_bounds.low) * (data_size - 1) / data_range);
    size_t end_idx = static_cast<size_t>((intersect_high - data_bounds.low) * (data_size - 1) / data_range);
    
    if (start_idx >= end_idx || end_idx >= data_size) {
        return;                            // Некорректные индексы
    }

    // Применение 5% отступа от краёв диапазона
    size_t margin = static_cast<size_t>((end_idx - start_idx) * 0.05);
    start_idx = std::min(start_idx + margin, end_idx - 1);
    end_idx = std::max(start_idx + 1, end_idx - margin);

    // Поиск минимального и максимального значения с использованием IPP
    float mean_val, max_val;
    IppStatus status = ippsMin_32f(&data[start_idx], static_cast<int>(end_idx - start_idx), &mean_val/*, IppHintAlgorithm::ippAlgHintFast*/);
    if (status != ippStsNoErr) {
        return;                            // Ошибка при расчёте минимума
    }
    
    status = ippsMax_32f(&data[start_idx], static_cast<int>(end_idx - start_idx), &max_val);
    if (status != ippStsNoErr) {
        return;                            // Ошибка при расчёте максимума
    }




    // Формирование новых границ с учётом запасов
    Limits<double> new_bounds{static_cast<double>(mean_val), static_cast<double>(max_val)};
    double delta = new_bounds.delta();
    new_bounds.low -= delta * power_margins_.low;   // Снижение нижней границы
    new_bounds.high += delta * power_margins_.high; // Повышение верхней границы

    // Объединение с текущими границами: расширение только наружу
    Limits<double> current_bounds = power_bounds_.load();
	if (!need_reset_bounds_) {
		new_bounds.low	= std::min(current_bounds.low, new_bounds.low);
		new_bounds.high = std::max(new_bounds.high, current_bounds.high);
	}	
    // Атомарное обновление границ мощности
    power_bounds_ = new_bounds;
	need_reset_bounds_ = false;
}

bool PowerLimitMan::IsAdaptiveMode() const {
    return is_adaptive_mode_.load();       // Проверка режима адаптивности
}

void PowerLimitMan::EnableAdaptiveMode(const bool is_true)
{
    is_adaptive_mode_ = is_true;           // Включение/отключение адаптивного режима
}

void aqua_gui::PowerLimitMan::ResetBounds()
{
	need_reset_bounds_ = true;
}

bool aqua_gui::PowerLimitMan::NeedRelevantBounds() const
{
	return need_reset_bounds_;
}
