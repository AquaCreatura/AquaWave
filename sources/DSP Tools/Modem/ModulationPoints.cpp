#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "ModulationPoints.h"
#include <string>
#pragma once
#include <vector>
#include <cmath>
#include <ipp.h>

void AddQAMFixed(std::vector<Ipp32fc>& points, int M)
{
	points.reserve(points.size() + M);

	switch (M)
	{
	case 4:
	{
		const Ipp32f s = 0.707106781f;
		points.push_back({ -s, -s });
		points.push_back({ -s,  s });
		points.push_back({ s, -s });
		points.push_back({ s,  s });
		break;
	}

	case 16:
	{
		const Ipp32f s = 0.316227766f;
		const Ipp32f a = 1.0f * s;
		const Ipp32f b = 3.0f * s;
		const Ipp32f qv[] = { -b, -a, a, b };
		const Ipp32f iv[] = { -b, -a, a, b };
		for (Ipp32f q : qv)
			for (Ipp32f i : iv)
				points.push_back({ i, q });
		break;
	}

	case 32:
	{
		const Ipp32f s = 0.223606798f;
		const Ipp32f qv[] = { -3.0f, -1.0f, 1.0f, 3.0f };
		const Ipp32f iv[] = { -5.0f, -3.0f, -1.0f, 1.0f, 3.0f, 5.0f };
		for (Ipp32f q : qv)
			for (Ipp32f i : iv)
				points.push_back({ i * s, q * s });
		const Ipp32f qv2[] = { -3.0f, -1.0f, 1.0f, 3.0f };
		const Ipp32f y5 = 5.0f * s;
		const Ipp32f yn5 = -5.0f * s;
		for (Ipp32f q : qv2)
		{
			points.push_back({ q * s, y5 });
			points.push_back({ q * s, yn5 });
		}
		break;
	}

	case 64:
	{
		const Ipp32f s = 0.154303350f;
		const Ipp32f v[] = { -7.0f, -5.0f, -3.0f, -1.0f, 1.0f, 3.0f, 5.0f, 7.0f };
		for (Ipp32f q : v)
			for (Ipp32f i : v)
				points.push_back({ i * s, q * s });
		break;
	}

	case 128:
	{
		const Ipp32f s = 0.110431526f;
		const int ni = 16;
		const int nq = 8;
		const int s0 = 4;
		for (int q = -(nq - 1); q <= nq - 1; q += 2)
		{
			for (int i = -(ni - 1); i <= ni - 1; i += 2)
			{
				int x = i;
				int y = q;
				if (std::abs(x) > 3 * s0 && std::abs(y) > s0)
				{
					x = (x > 0 ? 1 : -1) * (std::abs(x) - 2 * s0);
					y = (y > 0 ? 1 : -1) * (4 * s0 - std::abs(y));
				}
				else if (std::abs(x) > 3 * s0 && std::abs(y) <= s0)
				{
					x = (x > 0 ? 1 : -1) * (4 * s0 - std::abs(x));
					y = (y > 0 ? 1 : -1) * (std::abs(y) + 2 * s0);
				}
				points.push_back({ (Ipp32f)x * s, (Ipp32f)y * s });
			}
		}
		break;
	}

	case 256:
	{
		const Ipp32f s = 0.076696499f;
		for (int q = -15; q <= 15; q += 2)
			for (int i = -15; i <= 15; i += 2)
				points.push_back({ (Ipp32f)i * s, (Ipp32f)q * s });
		break;
	}

	default:
		break;
	}
}
std::vector<Ipp32fc> AddQAMConst(int M)
{
	const int bits = static_cast<int>(std::log2(M));
	if (bits < 2 || (1 << bits) != M)
		return {};

	std::vector<Ipp32fc> points;
	points.reserve(M);

	if ((bits & 1) == 0)
	{
		const int n = 1 << (bits / 2);

		for (int q = -(n - 1); q <= n - 1; q += 2)
			for (int i = -(n - 1); i <= n - 1; i += 2)
				points.push_back({ static_cast<Ipp32f>(i), static_cast<Ipp32f>(q) });
	}
	else
	{
		const int n = (bits - 1) / 2;
		const int ni = 1 << (n + 1);
		const int nq = 1 << n;
		const int s = 1 << (n - 1);

		for (int q = -(nq - 1); q <= nq - 1; q += 2)
		{
			for (int i = -(ni - 1); i <= ni - 1; i += 2)
			{
				int x = i;
				int y = q;

				if (std::abs(x) > 3 * s && std::abs(y) > s)
				{
					x = std::copysign(std::abs(x) - 2 * s, x);
					y = std::copysign(4 * s - std::abs(y), y);
				}
				else if (std::abs(x) > 3 * s)
				{
					x = std::copysign(4 * s - std::abs(x), x);
					y = std::copysign(std::abs(y) + 2 * s, y);
				}

				points.push_back({ static_cast<Ipp32f>(x), static_cast<Ipp32f>(y) });
			}
		}
	}

	double power = 0.0;
	for (const Ipp32fc& p : points)
		power += static_cast<double>(p.re) * p.re + static_cast<double>(p.im) * p.im;

	const Ipp32f norm = static_cast<Ipp32f>(std::sqrt(power / M));

	for (Ipp32fc& p : points)
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
		AddQAMFixed(points, M);
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
