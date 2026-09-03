#pragma once
#include <ipps.h>
#include <vector>

namespace aq_demod
{
	std::vector<Ipp32fc> CalculateModulationPivots(const char* modulation);
	double GetEuclidDist(Ipp32fc first, Ipp32fc second);
	Ipp32fc GetClosestSymbol(Ipp32fc passed, std::vector<Ipp32fc>& pivots);
}