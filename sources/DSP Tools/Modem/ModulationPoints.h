#pragma once

#include <ipps.h>

#include <string>
#include <vector>

namespace aq_demod
{
	// Per-modulation demodulator settings, tuned so the loops lock and hold the
	// constellation:
	//   pll      - carrier loop (Costas PLL) speed;
	//   tll      - symbol timing loop (Gardner TED) speed;
	//   eq_koeff - equaliser speed;
	//   eq_alg   - equaliser algorithm: "AUTO" (blind MMA warm-up, then decision-
	//              directed LMS), "LMS" (decision-directed) or "MMA"/"CMA" (blind).
	struct DmdSliderStructure
	{
		double pll = 5e-4;
		double tll = 0.03;
		double eq_koeff = 1e-4;
		const char* eq_alg = "AUTO";
	};

	// Optimal settings for the given modulation ("QPSK", "64QAM", "1024QAM", ...),
	// selected by constellation size. Unknown modulations get the default set.
	DmdSliderStructure GetDefaultSliderSetup(const char* modulation);

	// Constellation points (unit average power) for a modulation name.
	std::vector<Ipp32fc> CalculateModulationPivots(const char* modulation);

	// Squared Euclidean distance between two points.
	double GetEuclidDist(Ipp32fc first, Ipp32fc second);
	Ipp32fc GetClosestSymbol(Ipp32fc passed, std::vector<Ipp32fc>& pivots);

	// Bit combinations for a constellation of `size` points: 2^k strings of length
	// k (k = log2(size)) in binary counting order, aligned with the pivot order
	// returned by CalculateModulationPivots.
	std::vector<std::string> GetTestingManipVec(int size);
}
