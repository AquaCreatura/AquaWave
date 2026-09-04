#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "ModulationPoints.h"

#pragma once

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

	auto AddQAM = [&points](int M)
	{
		const int n = static_cast<int>(std::sqrt(M));
		if (n * n != M)
			return;

		// Уровни: -n+1, -n+3, ..., n-1
		// Нормировка средней мощности к 1.
		const double norm = std::sqrt(2.0 * (M - 1) / 3.0);

		points.reserve(M);

		for (int q = -(n - 1); q <= n - 1; q += 2)
		{
			for (int i = -(n - 1); i <= n - 1; i += 2)
			{
				points.push_back({
					static_cast<Ipp32f>(i / norm),
					static_cast<Ipp32f>(q / norm)
				});
			}
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
	else if (mod == "PSK8")
	{
		AddPSK(8, M_PI / 8.0);
	}
	else if (mod == "PSK16")
	{
		AddPSK(16, M_PI / 16.0);
	}
	else if (mod == "PSK32")
	{
		AddPSK(32, M_PI / 32.0);
	}
	else if (mod == "QAM16")
	{
		AddQAM(16);
	}
	else if (mod == "QAM64")
	{
		AddQAM(64);
	}
	else if (mod == "QAM256")
	{
		AddQAM(256);
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
