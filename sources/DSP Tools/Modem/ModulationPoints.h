#pragma once
#include <ipps.h>

#include <string>
#include <vector>

namespace aq_demod
{
	// Оптимальные для каждой модуляции настройки демодулятора:
	//   pll      - скорость ФАПЧ (восстановление несущей);
	//   tll      - скорость петли тактовой синхронизации (Gardner TED);
	//   eq_koeff - скорость эквалайзера;
	//   eq_alg   - алгоритм эквалайзера ("AUTO" по умолчанию, "LMS" по решению,
	//              "MMA"/"CMA" слепые). AUTO — warm-up: слепой MMA на старте,
	//              после захвата — решающий LMS с гистерезисом.
	// Значения подобраны так, чтобы петли захватывались и держали созвездие
	// для соответствующей модуляции. Плотные QAM (512/1024) требуют более
	// узкой ФАПЧ: решающий LMS на них плохо сходится с нуля, поэтому warm-up
	// начинает со слепого MMA.
	struct DmdSliderStructure
	{
		double pll = 5e-4;
		double tll = 0.03;
		double eq_koeff = 1e-4;
		const char* eq_alg = "AUTO";
	};

	// Возвращает оптимальный набор настроек для указанной модуляции
	// (например "QPSK", "8PSK", "64QAM", "1024QAM"). Тип определяется по
	// числу точек созвездия (CalculateModulationPivots). Для неизвестной
	// модуляции возвращает общий дефолт.
	DmdSliderStructure GetDefaultSliderSetup(const char* modulation);

	std::vector<Ipp32fc> CalculateModulationPivots(const char* modulation);
	double GetEuclidDist(Ipp32fc first, Ipp32fc second);
	Ipp32fc GetClosestSymbol(Ipp32fc passed, std::vector<Ipp32fc>& pivots);

	// Тестовый набор битовых комбинаций для созвездия из size точек.
	// Возвращает 2^k строк длиной k (k = log2(size)) в порядке двоичного счёта,
	// например для size = 32: "00000", "00001", "00010", ...
	// Порядок строк согласован с порядком точек CalculateModulationPivots.
	std::vector<std::string> GetTestingManipVec(int size);
}
