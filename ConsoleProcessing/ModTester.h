#pragma once

#include "ConsoleHelper.h"

#include <QTextStream>

namespace console
{
	// Console modulation of a data file (the -mod mode). The input bytes are
	// converted to bits, mapped to constellation points, upsampled and pulse-shaped
	// (and optionally noised), then written as a baseband int16 complex .pcm file
	// scaled to the full 16-bit range.
	class ModTester
	{
	public:
		// Runs the modulation and returns the process exit code.
		static int RunModulation(const ModParams& params, QTextStream& out);
	};
}
