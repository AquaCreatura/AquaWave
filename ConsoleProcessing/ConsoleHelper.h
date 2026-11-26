#pragma once

#include "aqua_defines.h"
#include "Utilities/utility_aqua.h"

#include <QString>
#include <QStringList>

#include <optional>

class QCommandLineParser;

namespace console
{
	// Command line option names, kept in one place.
	// Convention: <scope>_<quantity>, where quantity is fc (carrier, Hz),
	// fs (sample rate, Hz) or sr (symbol rate, Hz).
	namespace options
	{
		inline constexpr auto kDmd = "dmd";          // command: demodulate
		inline constexpr auto kMod = "mod";          // command: modulate
		inline constexpr auto kType = "type";        // modulation type
		inline constexpr auto kIn = "in";            // input file
		inline constexpr auto kOut = "out";          // output file
		inline constexpr auto kSymRate = "sym_rate"; // symbol rate (modulate)
		inline constexpr auto kNoise = "noise";      // noise level, dB
		inline constexpr auto kOutSize = "out_size"; // output size limit, bytes
		inline constexpr auto kFileFc = "file_fc";   // file carrier, Hz
		inline constexpr auto kFileFs = "file_fs";   // file sample rate, Hz
		inline constexpr auto kDmdFc = "dmd_fc";     // demodulation carrier, Hz
		inline constexpr auto kDmdSr = "dmd_sr";     // demodulation symbol rate, Hz
		inline constexpr auto kPll = "pll";          // carrier loop speed
		inline constexpr auto kTll = "tll";          // timing loop speed
		inline constexpr auto kEq = "eq";            // equaliser speed
		inline constexpr auto kEqAlg = "eq_alg";     // equaliser algorithm
		inline constexpr auto kEqTaps = "eq_taps";   // equaliser length, taps
	}

	// Equaliser algorithms available on the command line (--eq_alg).
	// AUTO - warm-up (blind MMA, then decision-directed LMS after lock);
	// MMA/CMA - blind (no decisions); LMS - decision-directed.
	namespace eq_algorithms
	{
		inline constexpr auto kAuto = "AUTO";
		inline constexpr auto kMma = "MMA";
		inline constexpr auto kCma = "CMA";
		inline constexpr auto kLms = "LMS";
	}

	// Modulation defaults for -mod.
	namespace mod_defaults
	{
		inline constexpr int64_t kSymbolRateHz = 1'000'000; // 1000 kHz
		inline constexpr double  kNoiseLevelDb = -1000.0;   // disabled
		inline constexpr int     kUpsample = 4;
	}

	// Parameters of the console demodulation mode:
	//   -dmd --type <Type> --in <File> --dmd_fc <Hz> --dmd_sr <Hz>
	//        [--file_fc <Hz>] [--file_fs <Hz>] [--pll <v>] [--tll <v>] [--eq <v>]
	//        [--eq_alg <AUTO|MMA|CMA|LMS>] [--eq_taps <n>]
	// --file_fc/--file_fs are optional (taken from the file name if omitted);
	// --pll/--tll/--eq are optional loop speeds; --eq_alg/--eq_taps select the
	// equaliser.
	struct DmdParams
	{
		QString modulation;                              // demodulation type, e.g. "QPSK"
		QString input_path;                              // path to the signal file (.pcm)

		utility_aqua::aqua_opt<int64_t> file_carrier_hz;      // optional, Hz
		utility_aqua::aqua_opt<int64_t> file_samplerate_hz;   // optional, Hz

		int64_t demod_carrier_hz{ 0 };                   // required, Hz
		int64_t demod_symbol_rate_hz{ 0 };               // required, Hz

		utility_aqua::aqua_opt<double> pll_speed;        // optional loop speeds
		utility_aqua::aqua_opt<double> tll_speed;
		utility_aqua::aqua_opt<double> eq_speed;

		QString eq_alg;                                  // optional (AUTO/MMA/CMA/LMS)
		int eq_taps{ 0 };                                // optional length (0 = default)
	};

	// Parameters of the console modulation mode:
	//   -mod --type <Type> [--in <File>] [--out <File>]
	//        [--sym_rate <Hz>] [--noise <dB>] [--out_size <bytes>]
	// The input file is optional: if it is missing or unreadable, a random 15 MB
	// file is generated. The symbol rate affects only the output file name (which
	// follows the project naming standard with carrier and sample rate). The noise
	// level defaults to -1000 dB (disabled).
	struct ModParams
	{
		QString modulation;                              // modulation type, e.g. "QPSK"
		QString input_path;                              // path to the input data file

		QString output_path;                             // output .pcm (generated if empty)

		int64_t symbol_rate_hz{ mod_defaults::kSymbolRateHz }; // Hz, output name only
		double  noise_level_db{ mod_defaults::kNoiseLevelDb }; // dB, -1000 = disabled
		int     upsample{ mod_defaults::kUpsample };           // samples per symbol
		int64_t out_size_bytes{ 0 };                           // 0 = unlimited
	};

	// Loop limits and defaults, used to describe pll/tll/eq in the help.
	struct LoopRanges
	{
		aqua::Limits<double> pll{ 0.0, 0.0 };
		aqua::Limits<double> tll{ 0.0, 0.0 };
		aqua::Limits<double> eq{ 0.0, 0.0 };
		double pll_default{ 0.0 };
		double tll_default{ 0.0 };
		double eq_default{ 0.0 };
		int eq_taps_default{ 21 };
	};

	// Console front-end: command line parsing, help text and dispatch to the
	// demodulation/modulation runners.
	class ConsoleHelper
	{
	public:
		// True if the arguments request a console mode (-dmd/-mod or help). Used to
		// avoid starting the graphical interface.
		static bool IsConsoleInvocation(const QStringList& arguments);

		// Console entry point: opens the output, parses the command line, prints the
		// help when requested and dispatches to the demodulation (-dmd) or modulation
		// (-mod) runner. `arguments` is the full list from QCoreApplication::arguments()
		// (its first element is the application name, as expected by
		// QCommandLineParser). Returns the process exit code.
		static int RunConsole(const QStringList& arguments);

	private:
		static void ConfigureParser(QCommandLineParser& parser, const LoopRanges& loops);
		static QString FullHelp(QCommandLineParser& parser, const LoopRanges& loops);

		// Extract and validate the parameters of a parsed command line.
		static std::optional<DmdParams> ExtractDmd(const QCommandLineParser& parser,
			const LoopRanges& loops, QString& error);
		static std::optional<ModParams> ExtractModulate(const QCommandLineParser& parser,
			QString& error);
	};
}
