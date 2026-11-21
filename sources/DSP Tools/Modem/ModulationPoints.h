#pragma once
#include <ipps.h>

#include <string>
#include <vector>

namespace aq_demod
{
	std::vector<Ipp32fc> CalculateModulationPivots(const char* modulation);
	double GetEuclidDist(Ipp32fc first, Ipp32fc second);
	Ipp32fc GetClosestSymbol(Ipp32fc passed, std::vector<Ipp32fc>& pivots);

	// Тестовый набор битовых комбинаций для созвездия из size точек.
	// Возвращает 2^k строк длиной k (k = log2(size)) в порядке двоичного счёта,
	// например для size = 32: "00000", "00001", "00010", ...
	// Порядок строк согласован с порядком точек CalculateModulationPivots.
	std::vector<std::string> GetTestingManipVec(int size);
}
