#include "ConsoleHelper.h"

#include "DmdTester.h"
#include "ModTester.h"

#include <QCommandLineParser>
#include <QFileInfo>
#include <QIODevice>
#include <QList>
#include <QTextStream>

#include <Windows.h>

#include <cmath>

namespace
{
	// ===== Console output =====

	// The application is a Windows-subsystem binary, where the CRT's stdout is not
	// opened (reopening it with freopen() triggers a debug assertion). Output is
	// written straight to a Win32 handle through this device.
	class ConsoleDevice : public QIODevice
	{
	public:
		explicit ConsoleDevice(HANDLE handle) : handle_(handle) {}

		bool isSequential() const override { return true; }

	protected:
		qint64 readData(char*, qint64) override { return -1; }

		qint64 writeData(const char* data, qint64 len) override
		{
			if (!handle_ || handle_ == INVALID_HANDLE_VALUE || len <= 0)
				return -1;
			DWORD written = 0;
			if (!::WriteFile(handle_, data, static_cast<DWORD>(len), &written, nullptr))
				return -1;
			return static_cast<qint64>(written);
		}

	private:
		HANDLE handle_;
	};

	// Returns a handle for the console output: the inherited one when stdout is
	// already redirected to a file or pipe, otherwise the parent console (or a newly
	// created one).
	HANDLE AcquireOutputHandle()
	{
		HANDLE handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
		if (handle && handle != INVALID_HANDLE_VALUE)
		{
			const DWORD type = ::GetFileType(handle);
			if (type == FILE_TYPE_DISK || type == FILE_TYPE_PIPE)
				return handle;
		}

		if (::AttachConsole(ATTACH_PARENT_PROCESS) || ::AllocConsole())
		{
			handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
			if (handle && handle != INVALID_HANDLE_VALUE)
				return handle;
		}

		return (handle && handle != INVALID_HANDLE_VALUE) ? handle : nullptr;
	}

	// ===== Option table (single source of truth for the parser and the help) =====

	enum class OptionGroup
	{
		Command,
		Common,
		Demod,
		Modulate
	};

	struct OptionInfo
	{
		const char* name;
		const char* value_name; // empty for flags
		QString description;
		OptionGroup group;
	};

	QString FormatNumber(double value)
	{
		return QString::number(value, 'g', 4);
	}

	QString FormatLimits(const aqua::Limits<double>& limits)
	{
		return QString("[%1 .. %2]").arg(FormatNumber(limits.low), FormatNumber(limits.high));
	}

	QList<OptionInfo> BuildOptions(const console::LoopRanges& loops)
	{
		return QList<OptionInfo>{
			{ console::options::kDmd, "",
				QStringLiteral("Run console demodulation (no windows are opened)."), OptionGroup::Command },
			{ console::options::kMod, "",
				QStringLiteral("Run console modulation (no windows are opened)."), OptionGroup::Command },
			{ console::options::kType, "type",
				QStringLiteral("Modulation type: BPSK, QPSK, 8PSK, 16QAM, 32QAM, 64QAM, 128QAM, 256QAM, 512QAM, 1024QAM, 2048QAM, 4096QAM."), OptionGroup::Common },
			{ console::options::kIn, "path",
				QStringLiteral("Input file. For -dmd it is a signal file (.pcm, complex int16 or float32), for -mod it is the data file to modulate (a random 15 MB file is generated if it is missing or unreadable)."), OptionGroup::Common },
			{ console::options::kOut, "path",
				QStringLiteral("Output .pcm file (-mod). Generated from the input file name if omitted."), OptionGroup::Modulate },
			{ console::options::kSymRate, "Hz",
				QStringLiteral("Symbol rate (-mod), Hz. Default %1 Hz. Affects only the output file name.")
					.arg(console::mod_defaults::kSymbolRateHz), OptionGroup::Modulate },
			{ console::options::kNoise, "dB",
				QStringLiteral("Noise level (-mod), dB relative to the signal power. Default %1 dB (noise disabled).")
					.arg(FormatNumber(console::mod_defaults::kNoiseLevelDb)), OptionGroup::Modulate },
			{ console::options::kOutSize, "bytes",
				QStringLiteral("Stop writing once the output .pcm reaches this size in bytes (-mod). 0 = unlimited (default)."), OptionGroup::Modulate },
			{ console::options::kFileFc, "Hz",
				QStringLiteral("File carrier frequency, Hz. Taken from the file name if omitted."), OptionGroup::Demod },
			{ console::options::kFileFs, "Hz",
				QStringLiteral("File sample rate, Hz. Taken from the file name if omitted."), OptionGroup::Demod },
			{ console::options::kDmdFc, "Hz",
				QStringLiteral("Carrier frequency for demodulation, Hz (required)."), OptionGroup::Demod },
			{ console::options::kDmdSr, "Hz",
				QStringLiteral("Symbol rate for demodulation, Hz (required)."), OptionGroup::Demod },
			{ console::options::kPll, "value",
				QStringLiteral("PLL loop speed. Range %1. Default depends on the modulation type (%2).")
					.arg(FormatLimits(loops.pll), FormatNumber(loops.pll_default)), OptionGroup::Demod },
			{ console::options::kTll, "value",
				QStringLiteral("Timing loop speed. Range %1. Default depends on the modulation type (%2).")
					.arg(FormatLimits(loops.tll), FormatNumber(loops.tll_default)), OptionGroup::Demod },
			{ console::options::kEq, "value",
				QStringLiteral("Equaliser speed. Range %1. Default depends on the modulation type (%2).")
					.arg(FormatLimits(loops.eq), FormatNumber(loops.eq_default)), OptionGroup::Demod },
			{ console::options::kEqAlg, "name",
				QStringLiteral("Equaliser algorithm: AUTO (warm-up MMA -> LMS, default), MMA or CMA (blind), LMS (decision-directed)."), OptionGroup::Demod },
			{ console::options::kEqTaps, "n",
				QStringLiteral("Equaliser length in taps (odd recommended). Default %1.")
					.arg(loops.eq_taps_default), OptionGroup::Demod },
		};
	}

	// One help entry: "  --name <value>   description".
	QString OptionLine(const OptionInfo& info)
	{
		const QString name = QString::fromLatin1(info.name);
		QString left = (info.group == OptionGroup::Command)
			? QStringLiteral("  -%1").arg(name)
			: QStringLiteral("  --%1").arg(name);
		if (info.value_name && *info.value_name)
			left += QStringLiteral(" <%1>").arg(QString::fromLatin1(info.value_name));
		return left.leftJustified(36, QLatin1Char(' ')) + info.description + QLatin1Char('\n');
	}

	// ===== Value parsing =====

	QString BadValue(const char* name, const QString& value)
	{
		return QStringLiteral("Cannot parse the value of --%1: \"%2\".")
			.arg(QString::fromLatin1(name), value);
	}

	// Frequency parsing: a value without a suffix is treated as Hz.
	bool ParseFrequency(const QString& text, int64_t& out_hz)
	{
		QString value = text.trimmed();
		if (value.isEmpty())
			return false;

		double scale = 1.0;
		const struct { const char* suffix; double scale; } units[] = {
			{ "GHz", 1.e9 }, { "MHz", 1.e6 }, { "kHz", 1.e3 }, { "Hz", 1.0 }
		};
		for (const auto& unit : units)
		{
			const QString suffix = QString::fromLatin1(unit.suffix);
			if (value.endsWith(suffix, Qt::CaseInsensitive))
			{
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

	bool ParseDouble(const QString& text, double& out_value)
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

	// Reads one optional loop speed and validates it against its range.
	bool ExtractLoop(const QCommandLineParser& parser, const char* name,
		const aqua::Limits<double>& limits, utility_aqua::aqua_opt<double>& out_speed, QString& error)
	{
		if (!parser.isSet(name))
			return true;

		double value = 0.0;
		if (!ParseDouble(parser.value(name), value))
		{
			error = BadValue(name, parser.value(name));
			return false;
		}
		if (!limits.has_inside(value))
		{
			error = QStringLiteral("The value of --%1 must be in the range %2.")
				.arg(QString::fromLatin1(name), FormatLimits(limits));
			return false;
		}
		out_speed = value;
		return true;
	}
}

namespace console
{
	bool ConsoleHelper::IsConsoleInvocation(const QStringList& arguments)
	{
		for (const QString& arg : arguments)
		{
			const QString a = arg.trimmed();
			if (a.compare("--dmd", Qt::CaseInsensitive) == 0 ||
				a.compare("-dmd", Qt::CaseInsensitive) == 0 ||
				a.compare("--mod", Qt::CaseInsensitive) == 0 ||
				a.compare("-mod", Qt::CaseInsensitive) == 0 ||
				a.compare("--help", Qt::CaseInsensitive) == 0 ||
				a.compare("-h", Qt::CaseInsensitive) == 0 ||
				a.compare("-?", Qt::CaseInsensitive) == 0)
				return true;
		}
		return false;
	}

	void ConsoleHelper::ConfigureParser(QCommandLineParser& parser, const LoopRanges& loops)
	{
		parser.setApplicationDescription(QStringLiteral(
			"AquaWave - signal spectral analysis, modulation and demodulation.\n"
			"Run without options to start the graphical interface."));
		parser.addHelpOption();
		// Single-dash words are whole option names: "-dmd" == "--dmd".
		parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

		for (const OptionInfo& info : BuildOptions(loops))
		{
			const QStringList names{ QString::fromLatin1(info.name) };
			const QString value_name = info.value_name ? QString::fromLatin1(info.value_name) : QString();
			parser.addOption(value_name.isEmpty()
				? QCommandLineOption(names, info.description)
				: QCommandLineOption(names, info.description, value_name));
		}

		parser.addPositionalArgument(QStringLiteral("file"),
			QStringLiteral("Signal file to open in the graphical interface."),
			QStringLiteral("[file]"));
	}

	QString ConsoleHelper::FullHelp(QCommandLineParser& parser, const LoopRanges& loops)
	{
		const QList<OptionInfo> options_list = BuildOptions(loops);

		QString help;
		help += parser.applicationDescription() + QStringLiteral("\n\n");

		help += QStringLiteral(
			"Usage:\n"
			"  AquaWave.exe [file]                Start the graphical interface.\n"
			"  AquaWave.exe <command> [options]   Run a console command.\n"
			"\n"
			"Commands:\n");
		for (const OptionInfo& info : options_list)
			if (info.group == OptionGroup::Command)
				help += OptionLine(info);

		help += QStringLiteral("\nCommon options:\n");
		for (const OptionInfo& info : options_list)
			if (info.group == OptionGroup::Common)
				help += OptionLine(info);
		help += QStringLiteral("  -h, --help                          Show this help.\n");

		help += QStringLiteral("\nDemodulation options (-dmd):\n");
		for (const OptionInfo& info : options_list)
			if (info.group == OptionGroup::Demod)
				help += OptionLine(info);

		help += QStringLiteral("\nModulation options (-mod):\n");
		for (const OptionInfo& info : options_list)
			if (info.group == OptionGroup::Modulate)
				help += OptionLine(info);

		help += QLatin1Char('\n') + QStringLiteral(
			"Notes:\n"
			"  Quote the file path if it contains spaces.\n"
			"  Frequency values accept the suffixes Hz/kHz/MHz/GHz.\n"
			"  If --file_fc/--file_fs are omitted they are taken from the file name,\n"
			"  e.g. \"... 11081.434200MHz 7000.000KHz.pcm\".\n"
			"  --eq_alg selects the equaliser: AUTO (warm-up MMA -> LMS, default),\n"
			"  MMA/CMA (blind) or LMS (decision-directed). In decision-directed mode a\n"
			"  reliability gate replaces unreliable decisions with a blind MMA update,\n"
			"  which prevents error propagation and lets the warm-up hold ~40 dB on\n"
			"  dense QAM (1024/2048/4096QAM).\n"
			"  -mod writes a baseband int16 complex (ipp16sc) .pcm file; its name\n"
			"  carries the carrier and sample rate (symbol rate times the oversampling\n"
			"  factor, 4 samples/symbol), e.g. \"data_QPSK 0.000000MHz 4000.000KHz.pcm\".\n"
			"  If the input file is missing or unreadable, a random 15 MB file is used.\n"
			"\n"
			"Examples:\n"
			"  AquaWave.exe -dmd --type QPSK --in \"iDirect 11081.434200MHz 7000.000KHz.pcm\" ^\n"
			"      --dmd_fc 11081405991 --dmd_sr 297454 --eq_alg MMA\n"
			"  AquaWave.exe -mod --type QAM32 --in data.bin --out qam32.pcm --noise -30\n"
			"\n"
			"During demodulation the progress is printed: percent processed, current SNR (dB),\n"
			"current frequency offset (Hz) and current symbol rate estimate (Hz).\n");
		return help;
	}

	std::optional<DmdParams> ConsoleHelper::ExtractDmd(const QCommandLineParser& parser,
		const LoopRanges& loops, QString& error)
	{
		DmdParams params;
		params.modulation = parser.value(options::kType).toUpper();
		params.input_path = parser.value(options::kIn);

		if (params.modulation.isEmpty())
		{
			error = QStringLiteral("The --type option is required.");
			return std::nullopt;
		}
		if (params.input_path.isEmpty())
		{
			error = QStringLiteral("The --in option is required.");
			return std::nullopt;
		}
		if (!QFileInfo::exists(params.input_path))
		{
			error = QStringLiteral("File not found: \"%1\".").arg(params.input_path);
			return std::nullopt;
		}

		int64_t hz = 0;
		if (parser.isSet(options::kFileFc))
		{
			if (!ParseFrequency(parser.value(options::kFileFc), hz))
			{
				error = BadValue(options::kFileFc, parser.value(options::kFileFc));
				return std::nullopt;
			}
			params.file_carrier_hz = hz;
		}
		if (parser.isSet(options::kFileFs))
		{
			if (!ParseFrequency(parser.value(options::kFileFs), hz) || hz <= 0)
			{
				error = BadValue(options::kFileFs, parser.value(options::kFileFs));
				return std::nullopt;
			}
			params.file_samplerate_hz = hz;
		}
		if (parser.isSet(options::kDmdFc))
		{
			// A zero carrier is valid: baseband signals are demodulated at 0 Hz.
			if (!ParseFrequency(parser.value(options::kDmdFc), hz) || hz < 0)
			{
				error = BadValue(options::kDmdFc, parser.value(options::kDmdFc));
				return std::nullopt;
			}
			params.demod_carrier_hz = hz;
		}
		else
		{
			error = QStringLiteral("The --dmd_fc option (demodulation carrier, Hz) is required.");
			return std::nullopt;
		}
		if (parser.isSet(options::kDmdSr))
		{
			if (!ParseFrequency(parser.value(options::kDmdSr), hz) || hz <= 0)
			{
				error = BadValue(options::kDmdSr, parser.value(options::kDmdSr));
				return std::nullopt;
			}
			params.demod_symbol_rate_hz = hz;
		}
		else
		{
			error = QStringLiteral("The --dmd_sr option (demodulation symbol rate, Hz) is required.");
			return std::nullopt;
		}

		if (!ExtractLoop(parser, options::kPll, loops.pll, params.pll_speed, error)) return std::nullopt;
		if (!ExtractLoop(parser, options::kTll, loops.tll, params.tll_speed, error)) return std::nullopt;
		if (!ExtractLoop(parser, options::kEq, loops.eq, params.eq_speed, error)) return std::nullopt;

		if (parser.isSet(options::kEqTaps))
		{
			bool ok = false;
			const int taps = parser.value(options::kEqTaps).trimmed().toInt(&ok);
			if (!ok || taps < 1 || taps > 101)
			{
				error = BadValue(options::kEqTaps, parser.value(options::kEqTaps));
				return std::nullopt;
			}
			params.eq_taps = taps;
		}

		params.eq_alg = parser.value(options::kEqAlg).toUpper();
		if (!params.eq_alg.isEmpty() &&
			params.eq_alg != QLatin1String(eq_algorithms::kAuto) &&
			params.eq_alg != QLatin1String(eq_algorithms::kMma) &&
			params.eq_alg != QLatin1String(eq_algorithms::kCma) &&
			params.eq_alg != QLatin1String(eq_algorithms::kLms))
		{
			error = QStringLiteral("Unknown --%1 value: \"%2\" (expected AUTO, MMA, CMA or LMS).")
				.arg(QString::fromLatin1(options::kEqAlg), parser.value(options::kEqAlg));
			return std::nullopt;
		}

		return params;
	}

	std::optional<ModParams> ConsoleHelper::ExtractModulate(const QCommandLineParser& parser,
		QString& error)
	{
		ModParams params;
		params.modulation = parser.value(options::kType).toUpper();
		params.input_path = parser.value(options::kIn);
		params.output_path = parser.value(options::kOut);

		if (params.modulation.isEmpty())
		{
			error = QStringLiteral("The --type option is required.");
			return std::nullopt;
		}
		// --in is optional: if the file is missing or invalid, ModTester generates a
		// random data file.

		int64_t hz = 0;
		if (parser.isSet(options::kSymRate))
		{
			if (!ParseFrequency(parser.value(options::kSymRate), hz) || hz <= 0)
			{
				error = BadValue(options::kSymRate, parser.value(options::kSymRate));
				return std::nullopt;
			}
			params.symbol_rate_hz = hz;
		}

		double noise = 0.0;
		if (parser.isSet(options::kNoise))
		{
			if (!ParseDouble(parser.value(options::kNoise), noise))
			{
				error = BadValue(options::kNoise, parser.value(options::kNoise));
				return std::nullopt;
			}
			params.noise_level_db = noise;
		}

		if (parser.isSet(options::kOutSize))
		{
			int64_t bytes = 0;
			if (!ParseFrequency(parser.value(options::kOutSize), bytes) || bytes < 0)
			{
				error = BadValue(options::kOutSize, parser.value(options::kOutSize));
				return std::nullopt;
			}
			params.out_size_bytes = bytes;
		}

		return params;
	}

	int ConsoleHelper::RunConsole(const QStringList& arguments)
	{
		ConsoleDevice device(AcquireOutputHandle());
		device.open(QIODevice::WriteOnly);
		QTextStream out(&device);

		const LoopRanges loops = DmdTester::GetLoopRanges();
		QCommandLineParser parser;
		ConfigureParser(parser, loops);

		auto report_error = [&](const QString& message) {
			out << "Error: " << message << "\n\n" << FullHelp(parser, loops) << "\n";
			out.flush();
			return 1;
		};

		// QCommandLineParser::parse() expects the application name as the first element.
		if (!parser.parse(arguments))
			return report_error(parser.errorText());

		if (parser.isSet(QStringLiteral("help")))
		{
			out << FullHelp(parser, loops) << "\n";
			out.flush();
			return 0;
		}

		if (parser.isSet(options::kMod))
		{
			QString error;
			const auto params = ExtractModulate(parser, error);
			if (!params)
				return report_error(error);
			return ModTester::RunModulation(*params, out);
		}

		if (!parser.isSet(options::kDmd))
			return report_error(QStringLiteral("Unknown console invocation."));

		QString error;
		const auto params = ExtractDmd(parser, loops, error);
		if (!params)
			return report_error(error);

		return DmdTester::RunDemodulation(*params, out);
	}
}
