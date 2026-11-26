#include "ModulationPoints.h"

#include <ipp.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace
{
	// Constellation order parsed from a modulation name: the number before the tag
	// ("1024" in "1024QAM", "8" in "8PSK"). Returns 0 if absent or unparsable.
	int ParseConstellationOrder(const std::string& name, const char* tag)
	{
		const size_t pos = name.find(tag);
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

	// Square or cross QAM constellation with unit average power. Returns an empty
	// vector for an invalid order (not a power of two, or 8-QAM which has no cross
	// geometry).
	std::vector<Ipp32fc> MakeQamConstellation(int M)
	{
		const int k = static_cast<int>(std::log2(M));
		if (k < 2 || (1 << k) != M || k == 3)
			return {};

		std::vector<Ipp32fc> points;
		points.reserve(M);

		int side = 0;
		int threshold = 0;
		if (k % 2 == 0)
		{
			// Square QAM (4, 16, 64, 256, ...): no corners are cut.
			side = 1 << (k / 2);
			threshold = side;
		}
		else
		{
			// Cross QAM (32, 128, 512, ...): cut the four corners.
			side = 3 * (1 << ((k - 3) / 2));
			threshold = 1 << ((k - 1) / 2);
		}

		const int max_coord = side - 1;
		for (int y = -max_coord; y <= max_coord; y += 2)
		{
			for (int x = -max_coord; x <= max_coord; x += 2)
			{
				if (std::abs(x) > threshold && std::abs(y) > threshold)
					continue;
				points.push_back({ static_cast<Ipp32f>(x), static_cast<Ipp32f>(y) });
			}
		}

		double power = 0.0;
		for (const Ipp32fc& point : points)
			power += static_cast<double>(point.re) * point.re + static_cast<double>(point.im) * point.im;

		const Ipp32f norm = static_cast<Ipp32f>(std::sqrt(power / M));
		for (Ipp32fc& point : points)
		{
			point.re /= norm;
			point.im /= norm;
		}

		return points;
	}

	// PSK constellation with unit power, the first point rotated by `phase`.
	std::vector<Ipp32fc> MakePskConstellation(int M, double phase)
	{
		std::vector<Ipp32fc> points;
		points.reserve(M);

		const double step = 2.0 * M_PI / M;
		for (int i = 0; i < M; ++i)
		{
			const double angle = phase + i * step;
			points.push_back({ static_cast<Ipp32f>(std::cos(angle)), static_cast<Ipp32f>(std::sin(angle)) });
		}
		return points;
	}
}

aq_demod::DmdSliderStructure aq_demod::GetDefaultSliderSetup(const char* modulation)
{
	// Presets keyed by constellation size, chosen so each modulation locks and holds.
	struct Preset
	{
		int points;
		double pll;
		double tll;
		double eq;
	};

	const Preset presets[] = {
		{    2, 2.0e-3, 0.03, 1.0e-4 }, // BPSK
		{    4, 2.0e-3, 0.03, 1.0e-4 }, // QPSK / OQPSK / DQPSK
		{    8, 2.0e-3, 0.03, 1.0e-4 }, // 8PSK
		{   16, 2.0e-3, 0.03, 2.0e-4 }, // 16QAM / 16PSK
		{   32, 3.0e-3, 0.03, 5.0e-4 }, // 32QAM (cross)
		{   64, 5.0e-4, 0.03, 2.0e-4 }, // 64QAM
		{  128, 2.0e-4, 0.03, 1.0e-4 }, // 128QAM (cross)
		{  256, 2.0e-4, 0.03, 1.0e-4 }, // 256QAM
		{  512, 1.0e-4, 0.04, 1.0e-4 }, // 512QAM (cross)
		{ 1024, 1.0e-4, 0.04, 1.0e-4 }, // 1024QAM
		{ 2048, 5.0e-4, 0.03, 1.0e-4 }, // 2048QAM (cross)
		{ 4096, 5.0e-4, 0.03, 1.0e-4 }, // 4096QAM
	};

	const int points = static_cast<int>(CalculateModulationPivots(modulation).size());
	for (const Preset& preset : presets)
	{
		if (preset.points == points)
			return { preset.pll, preset.tll, preset.eq, "AUTO" };
	}

	return DmdSliderStructure{};
}

std::vector<Ipp32fc> aq_demod::CalculateModulationPivots(const char* modulation)
{
	const std::string mod = modulation ? modulation : "";

	if (mod == "BPSK")
		return { { -1.0f, 0.0f }, { 1.0f, 0.0f } };

	if (mod == "QPSK" || mod == "OQPSK" || mod == "DQPSK")
		return MakePskConstellation(4, M_PI / 4.0);

	if (mod.find("QAM") != std::string::npos)
	{
		const int M = ParseConstellationOrder(mod, "QAM");
		return M > 0 ? MakeQamConstellation(M) : std::vector<Ipp32fc>{};
	}

	if (mod.find("PSK") != std::string::npos)
	{
		const int M = ParseConstellationOrder(mod, "PSK");
		return M > 0 ? MakePskConstellation(M, M_PI / M) : std::vector<Ipp32fc>{};
	}

	return {};
}

double aq_demod::GetEuclidDist(Ipp32fc first, Ipp32fc second)
{
	const double delta_re = static_cast<double>(first.re) - second.re;
	const double delta_im = static_cast<double>(first.im) - second.im;
	return delta_re * delta_re + delta_im * delta_im;
}

Ipp32fc aq_demod::GetClosestSymbol(Ipp32fc passed, std::vector<Ipp32fc>& pivots)
{
	if (pivots.empty())
		return Ipp32fc();

	Ipp32fc closest = pivots[0];
	double least_distance = GetEuclidDist(passed, pivots[0]);
	for (const Ipp32fc& pivot : pivots)
	{
		const double distance = GetEuclidDist(passed, pivot);
		if (distance < least_distance)
		{
			closest = pivot;
			least_distance = distance;
		}
	}
	return closest;
}

std::vector<std::string> aq_demod::GetTestingManipVec(int size)
{
	std::vector<std::string> manip;
	if (size < 2)
		return manip;

	int bits = 0;
	while ((1 << bits) < size)
		++bits;

	// The size must be a power of two, otherwise the bit set cannot cover it.
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
