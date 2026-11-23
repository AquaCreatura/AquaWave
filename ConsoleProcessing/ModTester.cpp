#define NOMINMAX

#include "ModTester.h"

#include "DSP Tools/Modem/Modulator/CcmModulator.h"
#include "DSP Tools/Modem/Modulator/GaussianNoiseMaker.h"
#include "Utilities/parse_tools.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QRandomGenerator>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace
{
	// Input read block size (in bytes).
	constexpr size_t kReadBlockBytes = 1u << 16;

	// Размер случайного файла данных, создаваемого при некорректном входе.
	constexpr int64_t kRandomFileBytes = 15 * 1024 * 1024; // 15 MB

	// Аргумент scaleFactor для ippsConvert_32f16s_Sfs - это правый сдвиг
	// (результат = src * 2^-scaleFactor), поэтому для домножения на 2^15
	// (полный 16-битный диапазон) он отрицательный.
	constexpr int kInt16ScaleFactor = -15;

	// True, если файл существует и открывается на чтение.
	bool IsReadableFile(const QString& path)
	{
		if (path.isEmpty() || !QFileInfo::exists(path))
			return false;
		std::ifstream probe(path.toLocal8Bit().constData(), std::ios::binary);
		return static_cast<bool>(probe);
	}

	// Создаёт в каталоге dir файл случайных данных на kRandomFileBytes байт.
	// Возвращает путь к файлу (пустая строка при ошибке).
	QString MakeRandomInputFile(const QString& dir)
	{
		const QString path = QDir(dir).absoluteFilePath(QStringLiteral("random_15MB.bin"));

		std::ofstream out(path.toLocal8Bit().constData(), std::ios::binary | std::ios::trunc);
		if (!out)
			return QString();

		QRandomGenerator& rng = *QRandomGenerator::global();
		std::vector<quint32> buffer(1u << 14); // 64 KB
		int64_t remaining = kRandomFileBytes;
		while (remaining > 0) {
			rng.fillRange(buffer.data(), static_cast<int>(buffer.size()));
			const int64_t block_bytes = static_cast<int64_t>(buffer.size() * sizeof(quint32));
			const int64_t to_write = std::min(remaining, block_bytes);
			out.write(reinterpret_cast<const char*>(buffer.data()),
				static_cast<std::streamsize>(to_write));
			remaining -= to_write;
		}
		return path;
	}

	// Имя выходного .pcm по стандарту проекта: "<base>_<Mod> <Fc>MHz <Fs>KHz.pcm".
	QString DefaultOutputName(const console::ModParams& params, const QString& input_path)
	{
		const QFileInfo input_info(input_path);
		const int64_t sample_rate_hz = params.symbol_rate_hz * params.upsample;

		// Комментарий формируем как QByteArray, чтобы не возвращать std::string
		// через границу Qt (сборка Qt msvc2017 и приложения v143 несовместимы по STL).
		const QByteArray comment =
			input_info.completeBaseName().toLocal8Bit() + '_' + params.modulation.toLocal8Bit();
		const std::string name = aqua_parse_tools::generate_filename(
			0, sample_rate_hz, comment.constData(), "pcm");

		return input_info.absoluteDir().absoluteFilePath(QString::fromLocal8Bit(name.c_str()));
	}

	// Масштабирует IQ к полному 16-битному диапазону и конвертирует в int16
	// (тот же приём, что в file_writer::SelectionWriter::SendData).
	void ConvertToInt16(std::vector<Ipp32fc>& iq, std::vector<Ipp16s>& out_shorts)
	{
		out_shorts.resize(iq.size() * 2);
		ippsConvert_32f16s_Sfs(reinterpret_cast<Ipp32f*>(iq.data()), out_shorts.data(),
			static_cast<int>(out_shorts.size()), ippRndNear, kInt16ScaleFactor);
	}
}

int console::ModTester::RunModulation(const ModParams& params, QTextStream& out)
{
	// Входной файл: при отсутствии или некорректности создаём случайный.
	QString input_path = params.file_path;
	if (!IsReadableFile(input_path)) {
		QString dir = QDir::currentPath();
		if (!input_path.isEmpty()) {
			const QString input_dir = QFileInfo(input_path).absolutePath();
			if (QDir(input_dir).exists())
				dir = input_dir;
		}

		const QString random_path = MakeRandomInputFile(dir);
		if (random_path.isEmpty()) {
			out << "Error: cannot read the input file and cannot create a random one.\n";
			out.flush();
			return 1;
		}
		input_path = random_path;
		out << "Input file is missing or invalid. Generated a random 15 MB file: "
			<< random_path << "\n";
	}

	aq_mod::CcmModulator modulator;
	const QByteArray modulation = params.modulation.toLatin1();
	if (!modulator.Init(modulation.constData(), params.upsample)) {
		out << "Error: unknown modulation type \"" << params.modulation << "\".\n";
		out.flush();
		return 1;
	}

	aq_mod::GaussianNoiseMaker noise;
	noise.SetNoiseLevel(params.noise_level_db);
	const bool noise_enabled = params.noise_level_db > aq_mod::GaussianNoiseMaker::kDisabledLevelDb;
	noise.setEnable(noise_enabled);

	const int bits_per_symbol = modulator.GetBitsPerSymbol();
	if (bits_per_symbol <= 0) {
		out << "Error: invalid modulation \"" << params.modulation << "\".\n";
		out.flush();
		return 1;
	}

	const QString out_path =
		params.out_path.isEmpty() ? DefaultOutputName(params, input_path) : params.out_path;

	QElapsedTimer timer;
	timer.start();

	out << "=== AquaWave console modulation ===\n";
	out << "Input:       " << input_path << "\n";
	out << "Type:        " << params.modulation << "\n";
	out << "Output:      " << out_path << "\n";
	out << "Format:      int16 complex (ipp16sc), full 16-bit range\n";
	out << "Symbol rate: " << params.symbol_rate_hz << " Hz\n";
	out << "Sample rate: " << (params.symbol_rate_hz * params.upsample)
		<< " Hz (" << params.upsample << " samples/symbol)\n";
	out << "Bits/symbol: " << bits_per_symbol << "\n";
	out << "Noise level: "
		<< (noise_enabled ? QString::number(params.noise_level_db, 'f', 2)
			: QStringLiteral("disabled"))
		<< " dB\n";
	out << "Out size:    "
		<< (params.out_size_bytes > 0 ? QString::number(params.out_size_bytes) + QStringLiteral(" bytes")
			: QStringLiteral("unlimited"))
		<< "\n";
	out << "-------------------------------------------------------------------------------\n";
	out.flush();

	std::ifstream input(input_path.toLocal8Bit().constData(), std::ios::binary);
	if (!input) {
		out << "Error: cannot open \"" << input_path << "\".\n";
		out.flush();
		return 1;
	}

	std::ofstream output(out_path.toLocal8Bit().constData(), std::ios::binary | std::ios::trunc);
	if (!output) {
		out << "Error: cannot create \"" << out_path << "\".\n";
		out.flush();
		return 1;
	}

	std::vector<uint8_t> read_bytes(kReadBlockBytes);
	std::vector<uint8_t> pending_bits;
	std::vector<uint8_t> block_bytes;
	std::vector<uint8_t> chunk;
	std::vector<Ipp32fc> iq;
	std::vector<Ipp16s> out_shorts;
	int64_t written_bytes = 0;
	const int64_t max_out_bytes = params.out_size_bytes;

	while (true) {
		input.read(reinterpret_cast<char*>(read_bytes.data()),
			static_cast<std::streamsize>(kReadBlockBytes));
		const std::streamsize bytes_read = input.gcount();
		if (bytes_read <= 0)
			break;

		block_bytes.assign(read_bytes.begin(), read_bytes.begin() + bytes_read);
		const std::vector<uint8_t> block_bits = aq_mod::CcmModulator::BytesToBits(block_bytes);
		pending_bits.insert(pending_bits.end(), block_bits.begin(), block_bits.end());

		// Неполные биты переносятся в следующий блок.
		const size_t usable = (pending_bits.size() / bits_per_symbol) * bits_per_symbol;
		if (usable == 0)
			continue;

		chunk.assign(pending_bits.begin(), pending_bits.begin() + usable);
		pending_bits.erase(pending_bits.begin(), pending_bits.begin() + usable);

		if (!modulator.Process(chunk, iq)) {
			out << "Error: modulation failed.\n";
			out.flush();
			return 1;
		}
		noise.Process(iq);

		if (iq.empty())
			continue;

		ConvertToInt16(iq, out_shorts);
		const int64_t block_bytes_out = static_cast<int64_t>(out_shorts.size() * sizeof(Ipp16s));
		if (max_out_bytes > 0 && written_bytes + block_bytes_out >= max_out_bytes) {
			// Пишем только остаток до заданного размера. Округляем вниз до
			// целого числа комплексных отсчётов int16 (4 байта).
			int64_t remaining = max_out_bytes - written_bytes;
			remaining -= remaining % 4;
			if (remaining > 0) {
				output.write(reinterpret_cast<const char*>(out_shorts.data()),
					static_cast<std::streamsize>(remaining));
				written_bytes += remaining;
			}
			break;
		}
		output.write(reinterpret_cast<const char*>(out_shorts.data()),
			static_cast<std::streamsize>(block_bytes_out));
		written_bytes += block_bytes_out;
	}

	output.flush();
	output.close();

	// Считаем записанное по факту: при ограничении --out_size хвост обрезается,
	// поэтому числа символов/отсчётов должны соответствовать файлу, а не входу.
	const int64_t written_samples = written_bytes / static_cast<int64_t>(sizeof(Ipp16s) * 2);
	const int64_t upsample = std::max(1, modulator.GetUpsample());

	out << "-------------------------------------------------------------------------------\n";
	out << "Symbols:     " << written_samples / upsample << "\n";
	out << "Samples:     " << written_samples << "\n";
	out << "Written:     " << written_bytes << " bytes\n";
	out << "Time:        " << QString::number(timer.elapsed() / 1000.0, 'f', 2) << " s\n";
	out << "Done.\n";
	out.flush();

	return 0;
}
