#pragma once
#include <ipps.h>
#include <vector>

namespace aq_demod
{
	std::vector<Ipp32fc> CalculateModulationPivots(const char* modulation);
}