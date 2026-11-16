#pragma once

#include "aqua_defines.h"
#include "Utilities/utility_aqua.h"

#include <QCommandLineParser>
#include <QFileInfo>
#include <QString>
#include <QStringList>

#include <cmath>
#include <cstdint>
#include <optional>

namespace console
{
	// Command line option names, kept in one place.
	namespace options
	{
		inline constexpr auto kDmd = "dmd";
		inline constexpr auto kType = "type";
		inline constexpr auto kFilePath = "file_path";
		inline constexpr auto kFileFc = "file_fc";
		inline constexpr auto kFileSr = "file_sr";
		inline constexpr auto kDmdFc = "dmd_fc";
		inline constexpr auto kDmdSr = "dmd_sr";
		inline constexpr auto kPll = "pll";
		inline constexpr auto kTll = "tll";
		inline constexpr auto kEq = "eq";
	}

	// Parameters of the console demodulation mode:
	//   -dmd --type <Type> --file_path <File> --dmd_fc <Hz> --dmd_sr <Hz>
	//        [--file_fc <Hz>] [--file_sr <Hz>] [--pll <v>] [--tll <v>] [--eq <v>]
	// file_fc/file_sr are optional (taken from the file name if omitted);
	// pll/tll/eq are optional loop speeds.
	struct DmdParams
	{
		QString modulation;                              // Demodulation type, e.g. "QPSK"
		QString file_path;                               // Path to the signal file (.pcm)

		utility_aqua::aqua_opt<int64_t> file_carrier_hz;      // Optional, Hz
		utility_aqua::aqua_opt<int64_t> file_samplerate_hz;   // Optional, Hz

		int64_t demod_carrier_hz{ 0 };                   // Required, Hz
		int64_t demod_symbol_rate_hz{ 0 };               // Required, Hz

		utility_aqua::aqua_opt<double> pll_speed;        // Optional loop speeds
		utility_aqua::aqua_opt<double> tll_speed;
		utility_aqua::aqua_opt<double> eq_speed;
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
	};

	// Command line parsing and the console help text.
	class ConsoleHelper
	{
	public:
		// True if the arguments request a console mode (-dmd or the help option).
		// Used to avoid starting the graphical interface.
		static bool IsConsoleInvocation(const QStringList& args)
		{
			for (const QString& arg : args) {
				const QString a = arg.trimmed();
				if (a.compare("--dmd", Qt::CaseInsensitive) == 0 ||
					a.compare("-dmd", Qt::CaseInsensitive) == 0 ||
					a.compare("--help", Qt::CaseInsensitive) == 0 ||
					a.compare("-h", Qt::CaseInsensitive) == 0 ||
					a.compare("-?", Qt::CaseInsensitive) == 0)
					return true;
			}
			return false;
		}

		// Fills the parser with all options of the application.
		static void ConfigureParser(QCommandLineParser& parser, const LoopRanges& loops)
		{
			parser.setApplicationDescription(QStringLiteral(
				"AquaWave - signal spectral analysis and demodulation.\n"
				"Run without options to start the graphical interface."));
			parser.addHelpOption();
			// Single dash words are whole option names: "-dmd" == "--dmd".
			parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

			parser.addOption(QCommandLineOption(QStringList{ options::kDmd },
				QStringLiteral("Run console demodulation (no windows are opened).")));

			parser.addOption(QCommandLineOption(QStringList{ options::kType },
				QStringLiteral("Modulation type: BPSK, QPSK, 8PSK, 16QAM, 32QAM, 64QAM, 128QAM, 256QAM."),
				QStringLiteral("type")));

			parser.addOption(QCommandLineOption(QStringList{ options::kFilePath },
				QStringLiteral("Path to the signal file (.pcm, complex int16 or float32)."),
				QStringLiteral("path")));

			parser.addOption(QCommandLineOption(QStringList{ options::kFileFc },
				QStringLiteral("File carrier frequency, Hz. Taken from the file name if omitted."),
				QStringLiteral("Hz")));

			parser.addOption(QCommandLineOption(QStringList{ options::kFileSr },
				QStringLiteral("File sample rate, Hz. Taken from the file name if omitted."),
				QStringLiteral("Hz")));

			parser.addOption(QCommandLineOption(QStringList{ options::kDmdFc },
				QStringLiteral("Carrier frequency for demodulation, Hz (required)."),
				QStringLiteral("Hz")));

			parser.addOption(QCommandLineOption(QStringList{ options::kDmdSr },
				QStringLiteral("Symbol rate for demodulation, Hz (required)."),
				QStringLiteral("Hz")));

			parser.addOption(QCommandLineOption(QStringList{ options::kPll },
				QStringLiteral("PLL loop speed. Range %1, default %2.")
					.arg(FormatLimits(loops.pll), FormatNumber(loops.pll_default)),
				QStringLiteral("value")));

			parser.addOption(QCommandLineOption(QStringList{ options::kTll },
				QStringLiteral("Timing loop speed. Range %1, default %2.")
					.arg(FormatLimits(loops.tll), FormatNumber(loops.tll_default)),
				QStringLiteral("value")));

			parser.addOption(QCommandLineOption(QStringList{ options::kEq },
				QStringLiteral("Equaliser speed. Range %1, default %2.")
					.arg(FormatLimits(loops.eq), FormatNumber(loops.eq_default)),
				QStringLiteral("value")));

			parser.addPositionalArgument(QStringLiteral("file"),
				QStringLiteral("Signal file to open in the graphical interface."),
				QStringLiteral("[file]"));
		}

		// Extracts and validates the demodulation parameters from a parsed parser.
		static std::optional<DmdParams> ExtractDmd(const QCommandLineParser& parser,
			const LoopRanges& loops, QString& error)
		{
			DmdParams params;
			params.modulation = parser.value(options::kType).toUpper();
			params.file_path = parser.value(options::kFilePath);

			if (params.modulation.isEmpty()) {
				error = QStringLiteral("The --type option is required.");
				return std::nullopt;
			}
			if (params.file_path.isEmpty()) {
				error = QStringLiteral("The --file_path option is required.");
				return std::nullopt;
			}
			if (!QFileInfo::exists(params.file_path)) {
				error = QStringLiteral("File not found: \"%1\".").arg(params.file_path);
				return std::nullopt;
			}

			int64_t hz = 0;
			if (parser.isSet(options::kFileFc)) {
				if (!ParseFrequency(parser.value(options::kFileFc), hz)) {
					error = BadValue(options::kFileFc, parser.value(options::kFileFc));
					return std::nullopt;
				}
				params.file_carrier_hz = hz;
			}
			if (parser.isSet(options::kFileSr)) {
				if (!ParseFrequency(parser.value(options::kFileSr), hz) || hz <= 0) {
					error = BadValue(options::kFileSr, parser.value(options::kFileSr));
					return std::nullopt;
				}
				params.file_samplerate_hz = hz;
			}
			if (parser.isSet(options::kDmdFc)) {
				if (!ParseFrequency(parser.value(options::kDmdFc), hz) || hz <= 0) {
					error = BadValue(options::kDmdFc, parser.value(options::kDmdFc));
					return std::nullopt;
				}
				params.demod_carrier_hz = hz;
			}
			else {
				error = QStringLiteral("The --dmd_fc option (demodulation carrier, Hz) is required.");
				return std::nullopt;
			}
			if (parser.isSet(options::kDmdSr)) {
				if (!ParseFrequency(parser.value(options::kDmdSr), hz) || hz <= 0) {
					error = BadValue(options::kDmdSr, parser.value(options::kDmdSr));
					return std::nullopt;
				}
				params.demod_symbol_rate_hz = hz;
			}
			else {
				error = QStringLiteral("The --dmd_sr option (demodulation symbol rate, Hz) is required.");
				return std::nullopt;
			}

			double value = 0.0;
			if (!ExtractLoop(parser, options::kPll, loops.pll, params.pll_speed, error)) return std::nullopt;
			if (!ExtractLoop(parser, options::kTll, loops.tll, params.tll_speed, error)) return std::nullopt;
			if (!ExtractLoop(parser, options::kEq, loops.eq, params.eq_speed, error)) return std::nullopt;

			return params;
		}

		// Full help: the generated option list plus the extra notes.
		static QString FullHelp(QCommandLineParser& parser)
		{
			return parser.helpText() + QLatin1Char('\n') + Notes();
		}

		static QString Notes()
		{
			return QStringLiteral(
				"Notes:\n"
				"  Quote the file path if it contains spaces.\n"
				"  Frequency values accept the suffixes Hz/kHz/MHz/GHz.\n"
				"  If --file_fc/--file_sr are omitted they are taken from the file name,\n"
				"  e.g. \"... 11081.434200MHz 7000.000KHz.pcm\".\n"
				"\n"
				"Example:\n"
				"  AquaWave.exe -dmd --type QPSK --file_path \"iDirect 11081.434200MHz 7000.000KHz.pcm\" ^\n"
				"      --dmd_fc 11081405991 --dmd_sr 297454\n"
				"\n"
				"During demodulation the progress is printed: percent processed, current SNR (dB),\n"
				"current frequency offset (Hz) and current symbol rate estimate (Hz).\n");
		}

	private:
		static bool ExtractLoop(const QCommandLineParser& parser, const char* name,
			const aqua::Limits<double>& limits, utility_aqua::aqua_opt<double>& out_speed, QString& error)
		{
			if (!parser.isSet(name))
				return true;

			double value = 0.0;
			if (!ParseDouble(parser.value(name), value)) {
				error = BadValue(name, parser.value(name));
				return false;
			}
			if (!limits.has_inside(value)) {
				error = QStringLiteral("The value of --%1 must be in the range %2.")
					.arg(QString::fromLatin1(name), FormatLimits(limits));
				return false;
			}
			out_speed = value;
			return true;
		}

		static QString BadValue(const char* name, const QString& value)
		{
			return QStringLiteral("Cannot parse the value of --%1: \"%2\".")
				.arg(QString::fromLatin1(name), value);
		}

		// Frequency parsing: a value without a suffix is treated as Hz.
		static bool ParseFrequency(const QString& text, int64_t& out_hz)
		{
			QString value = text.trimmed();
			if (value.isEmpty())
				return false;

			double scale = 1.0;
			const struct { const char* suffix; double scale; } units[] = {
				{ "GHz", 1.e9 }, { "MHz", 1.e6 }, { "kHz", 1.e3 }, { "Hz", 1.0 }
			};
			for (const auto& unit : units) {
				const QString suffix = QString::fromLatin1(unit.suffix);
				if (value.endsWith(suffix, Qt::CaseInsensitive)) {
					scale = unit.scale;
					value.chop(suffix.size());
					break;
				}
			}

			value = value.trimmed();
			value.replace(QLatin1Char(','), QLatin1Char('.'));

			bool ok = false;
			const double parsed = value.toDouble(&ok);
			if (!ok || !std::isfinite(parsed))
				return false;

			out_hz = static_cast<int64_t>(std::llround(parsed * scale));
			return true;
		}

		static bool ParseDouble(const QString& text, double& out_value)
		{
			QString value = text.trimmed();
			value.replace(QLatin1Char(','), QLatin1Char('.'));
			bool ok = false;
			const double parsed = value.toDouble(&ok);
			if (!ok || !std::isfinite(parsed))
				return false;
			out_value = parsed;
			return true;
		}

		static QString FormatNumber(double value)
		{
			return QString::number(value, 'g', 4);
		}

		static QString FormatLimits(const aqua::Limits<double>& limits)
		{
			return QString("[%1 .. %2]").arg(FormatNumber(limits.low), FormatNumber(limits.high));
		}
	};
}
