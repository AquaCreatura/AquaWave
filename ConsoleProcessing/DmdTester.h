#pragma once

#include "ConsoleHelper.h"

#include <QTextStream>

namespace console
{
	// Console demodulation of a signal file (the -dmd mode). Reads the file in
	// blocks, shifts the selected carrier to zero and resamples to 4 samples per
	// symbol, demodulates, and prints periodic statistics.
	class DmdTester
	{
	public:
		// Runs the demodulation and returns the process exit code.
		static int RunDemodulation(const DmdParams& params, QTextStream& out);

		// Loop limits and defaults read from a probe demodulator (used by the help).
		static LoopRanges GetLoopRanges();
	};
}
