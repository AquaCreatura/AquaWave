#ifndef RESAMPLER_TOOLS
#define RESAMPLER_TOOLS
#pragma once
#include <iostream>
#include <cmath>
#include <limits>

#include <utility>
#include <exception>

#include <vector>
#include <cstdint>
#include <ipps.h> // Предполагается, что здесь находятся объявления функций IPP


namespace aqua_resampler
{

// Функция для поиска дроби, максимально приближенной к отношению двух чисел.
// Параметры:
//   a            - числитель исходного отношения (целое число)
//   b            - знаменатель исходного отношения (целое число)
//   max_denom    - максимальное допустимое значение знаменателя искомой дроби (по умолчанию 100)
//   not_less_than- булевский флаг: если true, итоговая дробь не может быть меньше (a / b)
// Возвращает пару <p, q>, представляющую дробь p/q, максимально приближенную к значению (a / b).
static std::pair<int, int> FindBestFraction(const double target, const int max_denom = 100, const bool not_less_than = true) 
{   
    // Инициализируем переменные для хранения наилучшей найденной дроби и её ошибки.
    // best_num и best_denom - числитель и знаменатель лучшей найденной дроби.
    int best_num = 0, best_denom = 1;
    // Изначально устанавливаем ошибку как максимально возможное значение, чтобы любая реальная дробь имела меньшую ошибку.
    double best_error = std::numeric_limits<double>::max();

    // Перебираем все допустимые значения знаменателя от 1 до max_denom.
    for (int cur_denom = 1; cur_denom <= max_denom; ++cur_denom) {
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
        if (not_less_than && fraction < target) 
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


// Функция GenerateWindowKoeffs генерирует коэффициенты для окна фильтра низких частот.
// Параметры:
//   cutoff_freq    - частота среза фильтра (должна быть в интервале (0, 0.5))
//   coefs_num      - количество коэффициентов (должно быть > 8 и <= 1'000'000)
//   window_type    - тип окна, используемого для генерации коэффициентов (например, Hamming, Blackman и т.п.)
//   coefs_complex  - вектор, куда будут записаны сгенерированные комплексные коэффициенты
// Возвращает true, если коэффициенты сгенерированы успешно, иначе false.
static bool GenerateWindowKoeffs( const double          cutoff_freq,
                           const int             coefs_num,
                           const IppWinType      window_type,
                           std::vector<Ipp32fc>& coefs_complex )
{    
    // Проверка корректности входных параметров.
    // Частота среза должна находиться в диапазоне (0, 0.5)
    // Количество коэффициентов должно быть больше 8 и не превышать 1'000'000.
    if( cutoff_freq <= 0.0 || cutoff_freq >= 0.5 || coefs_num <= 8 || coefs_num > 1000000 )
    {
        //std::cerr << "Неверные входные параметры: cutoff_freq = " << cutoff_freq << ", coefs_num = " << coefs_num << std::endl;
        return false;
    }

    // Определение необходимого размера буфера для генерации коэффициентов
    int coefs_gen_buf_size = 0;
    IppStatus status = ippsFIRGenGetBufferSize(coefs_num, &coefs_gen_buf_size);
    if(status != ippStsNoErr)
    {
        //std::cerr << "Ошибка при получении размера буфера для генерации коэффициентов, статус = " << status << std::endl;
        return false;
    }

    // Выделяем буфер для генерации коэффициентов, используя вектор байт.
    std::vector<uint8_t> coefs_gen_buf(coefs_gen_buf_size);

    // Создаем векторы для хранения коэффициентов в формате double и float.
    std::vector<Ipp64f> coefs_real_double(coefs_num);
    std::vector<Ipp32f> coefs_real_float(coefs_num);

    // Резервируем место для комплексных коэффициентов.
    coefs_complex.resize(coefs_num);

    // Генерация коэффициентов низкочастотного фильтра.
    // ippTrue указывает на то, что коэффициенты должны быть симметричными.
    status = ippsFIRGenLowpass_64f(cutoff_freq, coefs_real_double.data(), coefs_num,
                                   window_type, ippTrue, coefs_gen_buf.data());
    if(status != ippStsNoErr)
    {
        //std::cerr << "Ошибка при генерации коэффициентов низкочастотного фильтра, статус = " << status << std::endl;
        return false;
    }

    // Преобразование коэффициентов из double в float.
    status = ippsConvert_64f32f(coefs_real_double.data(), coefs_real_float.data(), coefs_num);
    if(status != ippStsNoErr)
    {
        //std::cerr << "Ошибка при конвертации коэффициентов из double в float, статус = " << status << std::endl;
        return false;
    }


    // Преобразование вещественных коэффициентов в комплексный формат.
    // Второй параметр (nullptr) указывает, что мнимая часть коэффициентов равна нулю.
    status = ippsRealToCplx_32f(coefs_real_float.data(), nullptr, coefs_complex.data(), coefs_num);
    if(status != ippStsNoErr)
    {
        //std::cerr << "Ошибка при преобразовании вещественных коэффициентов в комплексный формат, статус = " << status << std::endl;
        return false;
    }

    // Если все этапы выполнены успешно, возвращаем true.
    return true;
}






}
#endif // RESAMPLER_TOOLS
