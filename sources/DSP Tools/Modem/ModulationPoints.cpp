#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "ModulationPoints.h"

#include <ipp.h>

#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#pragma once


namespace
{
	// Порядок созвездия (число точек) из имени модуляции, если он распознаётся:
	// число, стоящее перед тегом (например "1024" в "1024QAM" или "8" в "8PSK").
	// Возвращает 0, если тег отсутствует или число не парсится.
	int ParseConstellationOrder(const std::string& name, const char* tag)
	{
		const auto pos = name.find(tag);
		if (pos == std::string::npos)
			return 0;

		try
		{
			return std::stoi(name.substr(0, pos) + name.substr(pos + std::strlen(tag)));
		}
		catch (...)
		{
			return 0;
		}
	}
}


std::vector<Ipp32fc> AddQAMConst(int M)
{
	int k = static_cast<int>(std::log2(M));

	// Отсекаем невалидные M и 8-QAM (k=3), так как для него нет cross-геометрии
	if (k < 2 || (1 << k) != M || k == 3)
		return {};

	std::vector<Ipp32fc> points;
	points.reserve(M);

	int side, threshold;
	if (k % 2 == 0)
	{
		// Квадратные QAM (4, 16, 64, 256...)
		side = 1 << (k / 2);
		threshold = side; // Порог равен стороне. Условие abs(x) > side всегда ложно -> углы не режем
	}
	else
	{
		// Cross QAM (32, 128, 512...)
		side = 3 * (1 << ((k - 3) / 2));       // Общая сторона сетки (6, 12, 24...)
		threshold = 1 << ((k - 1) / 2);        // Порог для отрезания углов (4, 8, 16...)
	}

	int max_coord = side - 1;

	// Единый цикл для всех типов. Никаких лишних условий внутри.
	for (int y = -max_coord; y <= max_coord; y += 2)
	{
		for (int x = -max_coord; x <= max_coord; x += 2)
		{
			// Для квадратов это условие никогда не выполнится. 
			// Для Cross QAM оно отсечет ровно 4 уголка.
			if (std::abs(x) > threshold && std::abs(y) > threshold)
				continue;

			points.push_back({ static_cast<Ipp32f>(x), static_cast<Ipp32f>(y) });
		}
	}

	// Нормировка на единичную среднюю мощность
	double power = 0.0;
	for (const auto& p : points)
		power += static_cast<double>(p.re) * p.re + static_cast<double>(p.im) * p.im;

	Ipp32f norm = static_cast<Ipp32f>(std::sqrt(power / M));
	for (auto& p : points)
	{
		p.re /= norm;
		p.im /= norm;
	}

	return points;
}
aq_demod::DmdSliderStructure aq_demod::GetDefaultSliderSetup(const char* modulation)
{
	// Модуляцию определяем по числу точек созвездия: чуть медленнее, зато
	// наглядно и без отдельного разбора имени.
	const int points = static_cast<int>(CalculateModulationPivots(modulation).size());

	// Явный набор для каждой модуляции. Алгоритм эквалайзера по умолчанию —
	// AUTO (warm-up MMA -> LMS): слепой старт не даёт решающему алгоритму
	// разойтись до раскрытия созвездия, а после захвата LMS точнее.
	if (points == 2)     // BPSK
		return { 2.0e-3, 0.03, 1.0e-4, "AUTO" };
	if (points == 4)     // QPSK / OQPSK / DQPSK
		return { 2.0e-3, 0.03, 1.0e-4, "AUTO" };
	if (points == 8)     // 8PSK
		return { 2.0e-3, 0.03, 1.0e-4, "AUTO" };
	if (points == 16)    // 16QAM / 16PSK
		return { 2.0e-3, 0.03, 2.0e-4, "AUTO" };
	if (points == 32)    // 32QAM (cross)
		return { 3.0e-3, 0.03, 5.0e-4, "AUTO" };
	if (points == 64)    // 64QAM
		return { 5.0e-4, 0.03, 2.0e-4, "AUTO" };
	if (points == 128)   // 128QAM (cross)
		return { 2.0e-4, 0.03, 1.0e-4, "AUTO" };
	if (points == 256)   // 256QAM
		return { 2.0e-4, 0.03, 1.0e-4, "AUTO" };
	if (points == 512)   // 512QAM (cross)
		return { 1.0e-4, 0.04, 1.0e-4, "AUTO" };
	if (points == 1024)  // 1024QAM
		return { 1.0e-4, 0.04, 1.0e-4, "AUTO" };

	// Неизвестная модуляция — общий дефолт.
	return DmdSliderStructure{};
}

std::vector<Ipp32fc> aq_demod::CalculateModulationPivots(const char * modulation)
{
	const std::string mod = modulation ? modulation : "";
	std::vector<Ipp32fc> points;

	auto AddPSK = [&points](int M, double phase = 0.0)
	{
		const double step = 2.0 * M_PI / M;

		points.reserve(M);

		for (int i = 0; i < M; ++i)
		{
			const double a = phase + i * step;
			points.push_back({
				static_cast<Ipp32f>(std::cos(a)),
				static_cast<Ipp32f>(std::sin(a))
			});
		}
	};



	if (mod == "BPSK")
	{
		points = {
			{ -1.0f, 0.0f },
			{ 1.0f, 0.0f }
		};
	}
	else if (mod == "QPSK" || mod == "OQPSK" || mod == "DQPSK")
	{
		AddPSK(4, M_PI / 4.0);
	}
	else if (mod.find("QAM") != std::string::npos)
	{
		const int M = ParseConstellationOrder(mod, "QAM");
		if (M > 0)
			points = AddQAMConst(M);
	}
	else if (mod.find("PSK") != std::string::npos)
	{
		const int M = ParseConstellationOrder(mod, "PSK");
		if (M > 0)
			AddPSK(M, M_PI / M);
	}

	return points;
}

double aq_demod::GetEuclidDist(Ipp32fc piv_iter, Ipp32fc passed)
{
	double y_delta = (piv_iter.im - passed.im);
	double x_delta = (piv_iter.re - passed.re);
	double cur_euclid = y_delta * y_delta + x_delta * x_delta;
	return cur_euclid;
}

Ipp32fc aq_demod::GetClosestSymbol(Ipp32fc passed, std::vector<Ipp32fc>& pivots)
{
	if (pivots.empty()) return Ipp32fc();
	Ipp32fc best_symbol = pivots[0];
	double least_euclid = 1.e30;
	for (auto piv_iter : pivots) {
		auto cur_euclid = GetEuclidDist(passed, piv_iter);
		if (cur_euclid < least_euclid) {
			best_symbol = piv_iter;
			least_euclid = cur_euclid;
		}
	}
	return best_symbol;
}

std::vector<std::string> aq_demod::GetTestingManipVec(int size)
{
	std::vector<std::string> manip;
	if (size < 2)
		return manip;

	int bits = 0;
	while ((1 << bits) < size)
		++bits;

	// size должно быть степенью двойки, иначе набор битов не покроет созвездие.
	if ((1 << bits) != size)
		return manip;

	manip.reserve(size);
	for (int value = 0; value < size; ++value)
	{
		std::string combo(bits, '0');
		for (int bit = 0; bit < bits; ++bit)
		{
			if (value & (1 << (bits - 1 - bit)))
				combo[bit] = '1';
		}
		manip.push_back(std::move(combo));
	}
	return manip;
}
