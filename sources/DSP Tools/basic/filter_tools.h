#pragma once

#include <ipps.h>

#include <vector>

// Общие генераторы FIR-коэффициентов (низкочастотный и формирующий RRC),
// используемые ресемплерами и модемом.
namespace aqua_dsp_tools
{
	// Оценка длины FIR-фильтра (степень двойки) для ресемплинга.
	//   ratio             - отношение частот Fs_out / Fs_in.
	//   passband_fraction - полоса пропускания / полоса Найквиста (0..1).
	int GetFirLenPow2(double ratio, double passband_fraction = 0.9);

	// Коэффициенты ФНЧ, сгенерированные через окно (ippsFIRGenLowpass_64f).
	//   cutoff_freq   - нормированная частота среза (0..0.5).
	//   coefs_num     - число коэффициентов (> 8).
	//   window_type   - тип окна (например, ippWinBlackman).
	//   coefs_complex - результат: комплексные коэффициенты.
	bool GenerateWindowKoeffs(double cutoff_freq, int coefs_num, IppWinType window_type,
		std::vector<Ipp32fc>& coefs_complex);

	// Коэффициенты формирующего фильтра с корневым приподнятым косинусом (RRC).
	//   rolloff            - коэффициент сглаживания (0..1).
	//   samples_per_symbol - число отсчётов на символ.
	//   filter_len         - длина фильтра (число отсчётов, обычно span*sps+1).
	//   coefs_complex      - результат: комплексные коэффициенты с единичным
	//                        усилением по постоянной составляющей.
	bool GenerateRrcKoeffs(double rolloff, int samples_per_symbol, int filter_len,
		std::vector<Ipp32fc>& coefs_complex);
}
