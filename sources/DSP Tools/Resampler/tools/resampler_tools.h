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
static bool GenerateWindowKoeffs(double cutoff_freq,
	int coefs_num,
	IppWinType window_type,
	std::vector<Ipp32fc>& coefs_complex)
{
	if (cutoff_freq <= 0.0 || cutoff_freq >= 0.5 || coefs_num <= 8 || coefs_num > 1000000)
		return false;

	int buf_size = 0;
	if (ippsFIRGenGetBufferSize(coefs_num, &buf_size) != ippStsNoErr)
		return false;

	std::vector<uint8_t> buf(buf_size);
	std::vector<Ipp64f> real64(coefs_num);
	std::vector<Ipp32f> real32(coefs_num);
	coefs_complex.resize(coefs_num);

	if (ippsFIRGenLowpass_64f(cutoff_freq, real64.data(), coefs_num,
		window_type, ippTrue, buf.data()) != ippStsNoErr)
		return false;

	if (ippsConvert_64f32f(real64.data(), real32.data(), coefs_num) != ippStsNoErr)
		return false;

	if (ippsRealToCplx_32f(real32.data(), nullptr,
		coefs_complex.data(), coefs_num) != ippStsNoErr)
		return false;

	return true;
}






}
#endif // RESAMPLER_TOOLS
