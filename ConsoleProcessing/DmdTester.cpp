#define NOMINMAX

#include "DmdTester.h"
#include "ConsoleHelper.h"
#include "ModTester.h"

#include "DSP Tools/Modem/Demodulation/ContinuousDemodulator.h"
#include "DSP Tools/Resampler/ResamplerMan.h"
#include "Arks/File Source/helpers/FileReadCore.h"
#include "special_defs/file_souce_defs.h"
#include "Utilities/file_helpers.h"
#include "Utilities/parse_tools.h"

#include <QElapsedTimer>
#include <QIODevice>
#include <QTextStream>

#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace
{
	// The demodulator works at 4 samples per symbol.
	constexpr int kUpsample = 4;
	constexpr double kTwoPi = 2.0 * M_PI;
	// File read block size (in samples).
	constexpr size_t kReadBlockSamples = 1u << 16;

	// The application is built as a Windows-subsystem binary, where the CRT's
	// stdout is not opened (reopening it with freopen() triggers a debug assertion).
	// Output is written straight to a Win32 handle through this device.
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
	// already redirected to a file or pipe, otherwise the parent console (or a
	// newly created one).
	HANDLE AcquireOutputHandle()
	{
		HANDLE handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
		if (handle && handle != INVALID_HANDLE_VALUE) {
			const DWORD type = ::GetFileType(handle);
			if (type == FILE_TYPE_DISK || type == FILE_TYPE_PIPE)
				return handle;
		}

		if (::AttachConsole(ATTACH_PARENT_PROCESS) || ::AllocConsole()) {
			handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
			if (handle && handle != INVALID_HANDLE_VALUE)
				return handle;
		}

		return (handle && handle != INVALID_HANDLE_VALUE) ? handle : nullptr;
	}

	// Reads the pll/tll/eq limits and defaults from a probe demodulator.
	console::LoopRanges GetLoopRanges()
	{
		aq_demod::ContinuousDemodulator probe;
		console::LoopRanges loops;
		loops.pll = probe.GetPllSpeedLimits();
		loops.tll = probe.GetTimingSpeedLimits();
		loops.eq = probe.GetEqualiserSpeedLimits();
		loops.pll_default = probe.GetPllSpeedDefault();
		loops.tll_default = probe.GetTimingSpeedDefault();
		loops.eq_default = probe.GetEqualiserSpeedDefault();
		loops.eq_taps_default = probe.GetEqualiserTapsDefault();
		return loops;
	}

	// One statistics line: percent processed, SNR, frequency offset, symbol rate.
	void PrintStats(QTextStream& out, int percent,
		const aq_demod::ContinuousDemodulator& demodulator, int64_t nominal_symbol_rate_hz)
	{
		const double snr_db = demodulator.GetLastSNR();
		const double freq_offset_hz = demodulator.GetFreqOffset() / kTwoPi * nominal_symbol_rate_hz;
		const double ratio = demodulator.GetSymbolRateRatio();
		const double symbol_rate_hz = (ratio > 1.e-6)
			? nominal_symbol_rate_hz / ratio
			: static_cast<double>(nominal_symbol_rate_hz);

		out << QString("Progress: %1%   SNR = %2 dB   dFc = %3 Hz   Bd = %4 Hz")
			.arg(percent, 3)
			.arg(snr_db, 7, 'f', 2)
			.arg(freq_offset_hz, 10, 'f', 1)
			.arg(symbol_rate_hz, 12, 'f', 1)
			<< "\n";
		out.flush();
	}

	// Runs the demodulation itself and returns the exit code.
	int RunDemodulation(const console::DmdParams& params, QTextStream& out)
	{
		// File carrier and sample rate: use the given values, otherwise parse them
		// from the file name (e.g. "... 11081.434200MHz 7000.000KHz.pcm").
		// The carrier may legitimately be 0 (baseband file), so an explicitly
		// provided --file_fc 0 must not be treated as "missing".
		const QByteArray file_path_bytes = params.file_path.toLocal8Bit();
		const bool have_file_fc = static_cast<bool>(params.file_carrier_hz);
		const bool have_file_sr = static_cast<bool>(params.file_samplerate_hz);
		int64_t file_carrier_hz = params.file_carrier_hz.value_or(0);
		int64_t file_samplerate_hz = params.file_samplerate_hz.value_or(0);
		if (!have_file_fc || !have_file_sr) {
			int64_t parsed_carrier_hz = 0, parsed_samplerate_hz = 0;
			aqua_parse_tools::parse_filename(file_path_bytes.constData(), parsed_carrier_hz, parsed_samplerate_hz);
			if (!have_file_fc) file_carrier_hz = parsed_carrier_hz;
			if (!have_file_sr) file_samplerate_hz = parsed_samplerate_hz;
		}
		if (file_samplerate_hz <= 0) {
			out << "Error: cannot determine the file sample rate. "
				"Specify --file_sr explicitly.\n";
			out.flush();
			return 1;
		}

		// File description: the sample type is detected automatically (int16 or float32).
		aqua::SourceDescription file_descr;
		file_descr.file_name_ = params.file_path;
		file_descr.carrier_hz = file_carrier_hz;
		file_descr.samplerate_hz = file_samplerate_hz;
		file_descr.data_type_ = FsHelper::IsFile32Float(file_path_bytes.constData())
			? IppDataType::ipp32fc
			: IppDataType::ipp16sc;
		file_descr.bw_ratio_ = 0.9;
		file_descr.is_signal = true;
		file_descr.first_sample_offset = 0;

		const int64_t file_bytes = FsHelper::GetFileSize(file_path_bytes.constData());
		const size_t sample_size = file_source::GetSampleSize(file_descr.data_type_);
		if (file_bytes <= 0 || sample_size == 0) {
			out << "Error: cannot determine the size of \"" << params.file_path << "\".\n";
			out.flush();
			return 1;
		}
		file_descr.count_of_samples = file_bytes / static_cast<int64_t>(sample_size);

		file_source::StreamReader reader;
		if (!reader.SetFileParams(file_descr)) {
			out << "Error: cannot open \"" << params.file_path << "\".\n";
			out.flush();
			return 1;
		}
		if (!reader.InitStartEndRatio({ 0.0, 1.0 })) {
			out << "Error: cannot initialize file reading.\n";
			out.flush();
			return 1;
		}

		// Resampler: shift the selected carrier to zero and bring the file
		// to 4 samples per symbol relative to the symbol rate.
		aqua_resampler::ResamplerManager resampler;
		resampler.SetBaseParams(file_carrier_hz, file_samplerate_hz);
		int64_t demod_samplerate_hz =
			static_cast<int64_t>(std::llround(params.demod_symbol_rate_hz * kUpsample));
		const int64_t demod_bandwidth_hz = params.demod_symbol_rate_hz;
		if (!resampler.SetTargetParams(params.demod_carrier_hz, demod_samplerate_hz, demod_bandwidth_hz)) {
			out << "Error: cannot configure the resampler (check carriers and rates).\n";
			out.flush();
			return 1;
		}

		// Demodulator.
		aq_demod::ContinuousDemodulator demodulator;
		const QByteArray modulation = params.modulation.toLatin1();
		if (!demodulator.Init(modulation.constData(), kUpsample)) {
			out << "Error: unknown demodulation type \"" << params.modulation << "\".\n";
			out.flush();
			return 1;
		}
		// Дефолты скоростей петель зависят от типа модуляции: если в командной
		// строке параметр не задан, берём оптимальный набор для этой модуляции.
		const aq_demod::DmdSliderStructure setup =
			aq_demod::GetDefaultSliderSetup(modulation.constData());
		const double pll_speed = params.pll_speed.value_or(setup.pll);
		const double tll_speed = params.tll_speed.value_or(setup.tll);
		const double eq_speed = params.eq_speed.value_or(setup.eq_koeff);

		// Алгоритм эквалайзера тоже зависит от модуляции: по умолчанию AUTO
		// (слепой MMA на старте, затем решающий LMS после захвата).
		const QString eq_alg = params.eq_alg.isEmpty()
			? QString::fromLatin1(setup.eq_alg)
			: params.eq_alg;

		// Длину эквалайзера задаём до его скорости: шаг NLMS зависит от числа отводов.
		if (params.eq_taps > 0) demodulator.SetEqualiserTaps(params.eq_taps);
		demodulator.SetPllSpeed(pll_speed);
		demodulator.SetTimingSpeed(tll_speed);
		demodulator.SetEqualiserSpeed(eq_speed);
		demodulator.SetEqualiserAlgorithm(eq_alg.toLatin1().constData());

		// Header with the run parameters.
		out << "=== AquaWave console demodulation ===\n";
		out << "File:        " << params.file_path << "\n";
		out << "Type:        " << params.modulation << "\n";
		out << "Fc file:     " << file_carrier_hz << " Hz\n";
		out << "Fs file:     " << file_samplerate_hz << " Hz\n";
		out << "Fc demod:    " << params.demod_carrier_hz << " Hz\n";
		out << "Bd demod:    " << params.demod_symbol_rate_hz << " Hz\n";
		out << "Eq alg:      " << eq_alg << "\n";
		out << "Eq taps:     " << demodulator.GetEqualiserTaps() << "\n";
		out << "PLL/TLL/EQ:  " << QString::number(demodulator.GetPllSpeed(), 'g', 4)
			<< " / " << QString::number(demodulator.GetTimingSpeed(), 'g', 4)
			<< " / " << QString::number(demodulator.GetEqualiserSpeed(), 'g', 4) << "\n";
		out << "Demod Fs:    " << demod_samplerate_hz << " Hz (" << kUpsample << " samples/symbol)\n";
		out << "Samples:     " << file_descr.count_of_samples << "\n";
		out << "-------------------------------------------------------------------------------\n";
		out.flush();

		const int64_t total_samples = file_descr.count_of_samples;
		int64_t processed_samples = 0;
		int last_percent = -1;
		double settled_snr = 0.0;
		bool has_settled_snr = false;

		QElapsedTimer timer;
		timer.start();

		std::vector<uint8_t> read_bytes;
		std::vector<Ipp32fc> input_block;
		std::vector<Ipp32fc> synced;

		while (reader.IsReadStreamAvailable()) {
			if (!reader.ReadStream(read_bytes, kReadBlockSamples))
				break;

			const size_t samples_read = read_bytes.size() / sample_size;
			if (samples_read == 0)
				break;

			// int16 -> float32 (complex samples).
			input_block.resize(samples_read);
			if (file_descr.data_type_ == IppDataType::ipp16sc) {
				ippsConvert_16s32f(reinterpret_cast<Ipp16s*>(read_bytes.data()),
					reinterpret_cast<Ipp32f*>(input_block.data()),
					static_cast<int>(samples_read * 2));
			}
			else {
				std::memcpy(input_block.data(), read_bytes.data(), samples_read * sizeof(Ipp32fc));
			}

			// Carrier shift and conversion to 4 samples per symbol.
			if (resampler.ProcessBlock(input_block.data(), input_block.size())) {
				std::vector<Ipp32fc>& resampled = resampler.GetProcessedData();
				if (!resampled.empty())
					demodulator.SynchroniseIQ(resampled, synced);
			}

			processed_samples += static_cast<int64_t>(samples_read);

			// The settled SNR is accumulated after the first 20% of the file,
			// once the loops have locked.
			if (processed_samples > total_samples / 5) {
				const double snr = demodulator.GetLastSNR();
				if (std::isfinite(snr)) {
					settled_snr = has_settled_snr ? (settled_snr * 0.9 + snr * 0.1) : snr;
					has_settled_snr = true;
				}
			}

			const int percent = static_cast<int>(processed_samples * 100 / total_samples);
			if (percent != last_percent) {
				last_percent = percent;
				PrintStats(out, percent, demodulator, params.demod_symbol_rate_hz);
			}
		}

		// Final statistics.
		out << "-------------------------------------------------------------------------------\n";
		out << "Processed:   " << processed_samples << " / " << total_samples << " samples\n";
		out << "Time:        " << QString::number(timer.elapsed() / 1000.0, 'f', 2) << " s\n";
		PrintStats(out, 100, demodulator, params.demod_symbol_rate_hz);
		if (has_settled_snr)
			out << "SNR (settled, average): " << QString::number(settled_snr, 'f', 2) << " dB\n";
		if (demodulator.IsEqualiserWarmUp())
			out << "Eq warm-up:  "
				<< (demodulator.IsEqualiserLocked() ? "locked (LMS)" : "searching (MMA)")
				<< "\n";
		out.flush();

		return 0;
	}
}

int console::DmdTester::RunConsole(const QStringList& arguments)
{
	ConsoleDevice device(AcquireOutputHandle());
	device.open(QIODevice::WriteOnly);
	QTextStream out(&device);

	const LoopRanges loops = GetLoopRanges();
	QCommandLineParser parser;
	ConsoleHelper::ConfigureParser(parser, loops);

	// QCommandLineParser::parse() expects the application name as the first element.
	if (!parser.parse(arguments)) {
		out << "Error: " << parser.errorText() << "\n\n" << ConsoleHelper::FullHelp(parser, loops) << "\n";
		out.flush();
		return 1;
	}
	if (parser.isSet(QStringLiteral("help"))) {
		out << ConsoleHelper::FullHelp(parser, loops) << "\n";
		out.flush();
		return 0;
	}
	if (parser.isSet(options::kModulate)) {
		QString error;
		const auto params = ConsoleHelper::ExtractModulate(parser, error);
		if (!params) {
			out << "Error: " << error << "\n\n" << ConsoleHelper::FullHelp(parser, loops) << "\n";
			out.flush();
			return 1;
		}
		return ModTester::RunModulation(*params, out);
	}
	if (!parser.isSet(options::kDmd)) {
		out << "Error: unknown console invocation.\n\n" << ConsoleHelper::FullHelp(parser, loops) << "\n";
		out.flush();
		return 1;
	}

	QString error;
	const auto params = ConsoleHelper::ExtractDmd(parser, loops, error);
	if (!params) {
		out << "Error: " << error << "\n\n" << ConsoleHelper::FullHelp(parser, loops) << "\n";
		out.flush();
		return 1;
	}

	return RunDemodulation(*params, out);
}
