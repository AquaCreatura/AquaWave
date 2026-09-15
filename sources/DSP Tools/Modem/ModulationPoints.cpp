#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "ModulationPoints.h"
#include <string>
#pragma once
#include <vector>
#include <cmath>
#include <ipp.h>


#include <vector>
#include <cmath>
#include <ipp.h>

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
		const auto pos = mod.find("QAM");
		const int M = std::stoi(mod.substr(0, pos) + mod.substr(pos + 3));
		points = AddQAMConst(M);
	}
	else if (mod.find("PSK") != std::string::npos)
	{
		const auto pos = mod.find("PSK");
		const int M = std::stoi(mod.substr(0, pos) + mod.substr(pos + 3));
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
