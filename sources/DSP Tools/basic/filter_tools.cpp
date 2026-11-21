#include "filter_tools.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace
{
	constexpr double kPi = 3.14159265358979323846;

	// Значение импульсной характеристики RRC в точке t (в периодах символа).
	double RrcImpulse(double t, double beta)
	{
		const double at = std::fabs(t);

		// t = 0.
		if (at < 1e-8)
			return 1.0 - beta + 4.0 * beta / kPi;

		// Особая точка t = 1 / (4*beta): раскрытие неопределённости 0/0.
		if (beta > 0.0 && std::fabs(at - 1.0 / (4.0 * beta)) < 1e-6)
		{
			const double a = kPi / (4.0 * beta);
			return beta / std::sqrt(2.0) *
				((1.0 + 2.0 / kPi) * std::sin(a) - (1.0 - 2.0 / kPi) * std::cos(a));
		}

		const double num = std::sin(kPi * t * (1.0 - beta)) +
			4.0 * beta * t * std::cos(kPi * t * (1.0 + beta));
		const double den = kPi * t * (1.0 - std::pow(4.0 * beta * t, 2.0));
		return num / den;
	}
}

namespace aqua_dsp_tools
{

int GetFirLenPow2(double ratio, double passband_fraction)
{
	// ratio = Fs_out / Fs_in, passband_fraction = полоса пропускания / полоса Найквиста (0..1)
	if (ratio <= 0 || passband_fraction <= 0 || passband_fraction >= 1.0)
		return 0;
	passband_fraction = std::max(0.85, passband_fraction); //Меньшая частота среза - это странно...
	const double A = 100.0;                      // подавление, дБ
	double nyq_ratio = (ratio < 1.0) ? ratio * 0.5 : 0.5;
	double delta_f = nyq_ratio * (1.0 - passband_fraction); // ширина переходной полосы
	double N = (A - 8.0) / (14.36 * delta_f);   // порядок фильтра
	double L = N + 1.0;                         // длина
	return static_cast<int>(std::pow(2, std::ceil(std::log2(L))));
}

bool GenerateWindowKoeffs(double cutoff_freq, int coefs_num, IppWinType window_type,
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

bool GenerateRrcKoeffs(double rolloff, int samples_per_symbol, int filter_len,
	std::vector<Ipp32fc>& coefs_complex)
{
	if (samples_per_symbol < 1 || filter_len < 3 || rolloff < 0.0 || rolloff > 1.0)
		return false;

	const double center = (filter_len - 1) * 0.5;

	std::vector<Ipp32f> taps(filter_len);
	double sum = 0.0;
	for (int i = 0; i < filter_len; ++i)
	{
		const double t = (i - center) / static_cast<double>(samples_per_symbol);
		const double h = RrcImpulse(t, rolloff);
		taps[i] = static_cast<Ipp32f>(h);
		sum += h;
	}

	if (std::fabs(sum) < 1e-12)
		return false;

	// Нормировка на единичное усиление по постоянной составляющей.
	const Ipp32f norm = static_cast<Ipp32f>(1.0 / sum);
	for (int i = 0; i < filter_len; ++i)
		taps[i] *= norm;

	coefs_complex.resize(filter_len);
	if (ippsRealToCplx_32f(taps.data(), nullptr, coefs_complex.data(), filter_len) != ippStsNoErr)
		return false;

	return true;
}

}
