#pragma once

#include "aqua_defines.h"
#include <vector>
#include <atomic>

using namespace fluctus;

namespace aqua_gui
{

class PowerLimitMan
{
public:
    PowerLimitMan();  // Конструктор: инициализация пределов мощности и режима

    // Установка новых границ отображаемого диапазона
    void SetNewViewBounds(const Limits<double>& x_bounds);

    // Ручная установка границ мощности
    void SetPowerBounds(const Limits<double>& power_bounds);

    // Установка долей (margins) для расширения границ мощности
    void SetPowerMargins(const Limits<double>& power_margins);

    // Получение текущих границ мощности
    Limits<double> GetPowerBounds() const;

    // Обновление границ мощности на основе данных и их диапазона
    void UpdateBounds(const std::vector<float>& data, const Limits<double>& data_bounds);

    // Проверка, включён ли адаптивный режим
    bool IsAdaptiveMode() const;

    // Включение/отключение адаптивного режима
    void EnableAdaptiveMode(const bool is_true);

	void ResetBounds();

	bool NeedRelevantBounds() const;

protected:
    Limits<double> x_bounds_;                         // Зарезервировано (не используется в реализации)
    std::atomic_bool is_adaptive_mode_ = false;       // Флаг адаптивного режима
	std::atomic_bool need_reset_bounds_		= false;       
	std::atomic_bool need_update_max_		= false;       

    std::atomic<Limits<double>> power_bounds_;        // Актуальные границы мощности
    std::atomic<Limits<double>> view_bounds_;         // Видимые границы (используются при адаптации)
    Limits<double> power_margins_;                    // Запасы (margins) для изменения границ мощности
};

}
