#pragma once

#include <QStringList>

namespace console
{
	// Console demodulation of a signal file (the -dmd mode).
	// No windows are created: the file is read in chunks, converted to 4 samples
	// per symbol and processed by the demodulator with periodic statistics output.
	class DmdTester
	{
	public:
		// Entry point of the console mode. Parses the command line, prints the help
		// when requested and runs the demodulation. Returns the process exit code.
		// arguments is the full list from QCoreApplication::arguments() (its first
		// element is the application name, as expected by QCommandLineParser).
		static int RunConsole(const QStringList& arguments);
	};
}
