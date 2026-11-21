#pragma once

#include <QStringList>

namespace console
{
	// Entry point of the console modes (-dmd demodulation, -modulate modulation).
	// No windows are created: the file is read in chunks and processed with
	// periodic statistics output.
	class DmdTester
	{
	public:
		// Entry point of the console mode. Parses the command line, prints the help
		// when requested and dispatches to the demodulation (-dmd) or modulation
		// (-modulate) runner. Returns the process exit code.
		// arguments is the full list from QCoreApplication::arguments() (its first
		// element is the application name, as expected by QCommandLineParser).
		static int RunConsole(const QStringList& arguments);
	};
}
