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


// Функция GenerateWindowKoeffs генерирует коэффициенты для окна фильтра низких частот.
// Параметры:
//   cutoff_ratio    - частота среза фильтра (должна быть в интервале (0, 0.5))
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



// Оценка длины FIR-фильтра (степень 2) для ресемплинга
static int GetFirPow2(double ratio, double passband_fraction) {
	// ratio = Fs_out / Fs_in, passband_fraction = полоса пропускания / полоса Найквиста (0..1)
	if (ratio <= 0 || passband_fraction <= 0 || passband_fraction >= 1.0)
		return 0;

	const double A = 60.0;                      // подавление, дБ
	double min_ratio = (ratio < 1.0) ? ratio : 1.0;
	double delta_f = min_ratio * (1.0 - passband_fraction); // ширина переходной полосы
	double N = (A - 8.0) / (14.36 * delta_f);   // порядок фильтра
	double L = N + 1.0;                         // длина
	return static_cast<int>(std::pow(2, std::ceil(std::log2(L))));
}


}
#endif // RESAMPLER_TOOLS
