#ifndef RESAMPLER_TOOLS
#define RESAMPLER_TOOLS
#pragma once
#include "../../basic/filter_tools.h"

#include <ipps.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace aqua_resampler
{

// Функция для поиска дроби, максимально приближенной к отношению двух чисел.
// Параметры:
//   a            - числитель исходного отношения (целое число)
//   b            - знаменатель исходного отношения (целое число)
//   max_num_denom    - максимальное допустимое значение знаменателя искомой дроби 
//   not_less_than- булевский флаг: если true, итоговая дробь не может быть меньше (a / b)
// Возвращает пару <p, q>, представляющую дробь p/q, максимально приближенную к значению (a / b).
static std::pair<int, int> FindBestFraction(const double target, const int max_num_denom = 20, const bool not_less_than = true) 
{   
    // Инициализируем переменные для хранения наилучшей найденной дроби и её ошибки.
    // best_num и best_denom - числитель и знаменатель лучшей найденной дроби.
    int best_num = 0, best_denom = 1;
    // Изначально устанавливаем ошибку как максимально возможное значение, чтобы любая реальная дробь имела меньшую ошибку.
    double best_error = std::numeric_limits<double>::max();

    // Перебираем все допустимые значения знаменателя от 1 до max_num_denom.
    for (int cur_denom = 1; cur_denom <= max_num_denom; ++cur_denom) {
        int cur_num;
        // Для каждого знаменателя вычисляем подходящий числитель.
        // Если установлен флаг not_less_than, выбираем числитель таким образом, чтобы дробь cur_num/cur_denom не была меньше целевого отношения.
        if (not_less_than) 
        {
            // Используем округление вверх: ceil(target * cur_denom).
            cur_num = static_cast<int>(std::ceil(target * cur_denom));
        } else 
        {
            // Иначе выбираем числитель, округляя до ближайшего целого: round(target * cur_denom).
            cur_num = static_cast<int>(std::round(target * cur_denom));
        }

        // Вычисляем значение дроби для текущего cur_num и cur_denom.
        double fraction = static_cast<double>(cur_num) / cur_denom;
        // Если установлен флаг not_less_than и дробь оказывается меньше целевого, пропускаем её.
		////По факту (not_less_than && fraction < target) всегда false, т.к. округляем вверх
        if ((cur_num > max_num_denom) || (not_less_than && fraction < target)) 
        {
            continue;
        }
        
        // Вычисляем абсолютную разницу между полученной дробью и целевым значением.
        const double error = std::fabs(fraction - target);
        
        // Если текущая ошибка меньше, чем ранее зафиксированная, обновляем лучший вариант.
        if (error < best_error) 
        {
            best_error = error;
            best_num = cur_num;
            best_denom = cur_denom;
        }
    }    
    // Возвращаем найденную дробь в виде пары: числитель и знаменатель.
    return std::make_pair(best_num, best_denom);
}

// Генераторы коэффициентов (ФНЧ с окном и формирующий RRC) вынесены в
// "../../basic/filter_tools.h" (namespace aqua_dsp_tools) и общие для
// ресемплеров и модема.

}
#endif // RESAMPLER_TOOLS
