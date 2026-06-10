#include "wav_tools.h"

using namespace utility_aqua;
bool utility_aqua::get_wav_info(const std::string & filename, WavInfo & out)
{
	std::ifstream file(filename, std::ios::binary);
	if (!file)
		return false;

	auto read_u16 = [&](uint16_t& v) -> bool {
		return static_cast<bool>(file.read(reinterpret_cast<char*>(&v), 2));
	};
	auto read_u32 = [&](uint32_t& v) -> bool {
		return static_cast<bool>(file.read(reinterpret_cast<char*>(&v), 4));
	};
	auto tell = [&]() -> uint64_t {
		return static_cast<uint64_t>(file.tellg());
	};
	auto seek = [&](uint64_t pos) -> bool {
		file.seekg(static_cast<std::streamoff>(pos), std::ios::beg);
		return static_cast<bool>(file);
	};

	// --- RIFF header ---
	char riff[12];
	if (!file.read(riff, 12))
		return false;
	if (std::memcmp(riff, "RIFF", 4) != 0 || std::memcmp(riff + 8, "WAVE", 4) != 0)
		return false;

	// Размер файла (для проверок)
	file.seekg(0, std::ios::end);
	uint64_t file_size = static_cast<uint64_t>(file.tellg());
	file.seekg(12, std::ios::beg);

	bool fmt_found = false;
	bool data_found = false;

	while (file)
	{
		char chunk_id[4];
		uint32_t chunk_size;

		if (!file.read(chunk_id, 4))
			break;
		if (!read_u32(chunk_size))
			break;

		uint64_t chunk_data_pos = tell();

		// Защита от выхода за пределы файла
		if (chunk_data_pos > file_size || chunk_size > file_size - chunk_data_pos)
			return false;

		if (std::memcmp(chunk_id, "fmt ", 4) == 0)
		{
			if (chunk_size < 16)
			{
				seek(chunk_data_pos + chunk_size + (chunk_size & 1));
				continue;
			}

			uint16_t audio_format, num_channels;
			uint32_t sample_rate, byte_rate;
			uint16_t block_align, bits_per_sample;

			if (!read_u16(audio_format)) break;
			if (!read_u16(num_channels)) break;
			if (!read_u32(sample_rate)) break;
			if (!read_u32(byte_rate)) break;
			if (!read_u16(block_align)) break;
			if (!read_u16(bits_per_sample)) break;

			out.audio_format = audio_format;
			out.num_channels = num_channels;
			out.sample_rate = sample_rate;
			out.byte_rate = byte_rate;
			out.block_align = block_align;
			out.bits_per_sample = bits_per_sample;

			fmt_found = true;
			seek(chunk_data_pos + chunk_size);
		}
		else if (std::memcmp(chunk_id, "data", 4) == 0)
		{
			out.data_offset = static_cast<uint32_t>(chunk_data_pos);  // или сохранить uint64_t, если нужно
			out.data_size = chunk_size;
			data_found = true;
			seek(chunk_data_pos + chunk_size);
		}
		else
		{
			seek(chunk_data_pos + chunk_size);
		}

		// RIFF padding (чётное выравнивание)
		if (chunk_size & 1)
		{
			if (!seek(tell() + 1))
				break;
		}

		if (fmt_found && data_found)
			break;
	}

	return fmt_found && data_found;  // только если есть и частота, и данные
}

std::string utility_aqua::MakeWavPrefix(const WavInfo & info)

{
	std::string out;
	out.resize(44);

	char* p = out.data();

	auto write_u16 = [&](uint16_t v)
	{
		*p++ = static_cast<char>(v & 0xFF);
		*p++ = static_cast<char>((v >> 8) & 0xFF);
	};

	auto write_u32 = [&](uint32_t v)
	{
		*p++ = static_cast<char>(v & 0xFF);
		*p++ = static_cast<char>((v >> 8) & 0xFF);
		*p++ = static_cast<char>((v >> 16) & 0xFF);
		*p++ = static_cast<char>((v >> 24) & 0xFF);
	};

	// RIFF
	std::memcpy(p, "RIFF", 4);
	p += 4;

	// ИСПРАВЛЕНО: Теперь размер RIFF-чанка динамический и корректный
	// Если размер файлов неизвестен (стрим), лучше передавать info.data_size = 0xFFFFFFFF
	if (info.data_size.value() == 0xFFFFFFFF) {
		write_u32(0xFFFFFFFF);
	}
	else {
		write_u32(36 + info.data_size);
	}

	std::memcpy(p, "WAVE", 4);
	p += 4;

	// fmt chunk
	std::memcpy(p, "fmt ", 4);
	p += 4;

	write_u32(16); // PCM fmt size

	write_u16(info.audio_format);
	write_u16(info.num_channels);
	write_u32(info.sample_rate);
	write_u32(info.byte_rate);
	write_u16(info.block_align);
	write_u16(info.bits_per_sample);

	// data chunk
	std::memcpy(p, "data", 4);
	p += 4;

	write_u32(info.data_size);

	return out;
}

WavInfo utility_aqua::DefaultWav(const int64_t SampleRate)

{
	WavInfo info;

	info.audio_format = 1;       // PCM
	info.num_channels = 1;       // mono
	info.bits_per_sample = 16;
	info.sample_rate = static_cast<uint32_t>(SampleRate);

	info.block_align = (info.num_channels * info.bits_per_sample) / 8;
	info.byte_rate = info.sample_rate * info.block_align;

	info.data_offset = 44;
	info.data_size = 0; // Для пустого шаблона это нормально, RIFF запишет 36
	return info;
}

bool utility_aqua::UpdateWavDataSize(const std::string & file_path)

{
	// Открываем файл в режиме чтение+запись БЕЗ усечения (ios::ate или ios::trunc НЕ используем)
	std::fstream file(file_path, std::ios::in | std::ios::out | std::ios::binary);

	if (!file.is_open()) {
		return false; // Файл не нашелся или занят другим процессом
	}

	// 1. Узнаем реальный размер получившегося файла на диске
	file.seekg(0, std::ios::end);
	std::streamoff file_size = static_cast<std::streamoff>(file.tellg());
	// Если файл почему-то меньше заголовка, значит произошла ошибка при записи звука
	if (file_size < 44) {
		return false;
	}

	// Размер чистых аудиоданных — это весь файл минус 44 байта заголовка
	uint32_t data_size = static_cast<uint32_t>(file_size - 44);
	uint32_t riff_size = 36 + data_size;

	// Лямбда для точечной записи u32 без сдвига окружающих байт
	auto write_u32_at = [&](std::streamoff offset, uint32_t v)
	{
		char buf[4];
		buf[0] = static_cast<char>(v & 0xFF);
		buf[1] = static_cast<char>((v >> 8) & 0xFF);
		buf[2] = static_cast<char>((v >> 16) & 0xFF);
		buf[3] = static_cast<char>((v >> 24) & 0xFF);

		file.seekp(offset); // Встаем строго на нужную позицию
		file.write(buf, 4);  // Заменяем 4 байта старых данных на новые
	};

	// 2. Обновляем размер RIFF-чанка (смещение 4)
	write_u32_at(4, riff_size);

	// 3. Обновляем размер DATA-чанка (смещение 40)
	write_u32_at(40, data_size);

	// Файл закрывается автоматически, данные сохранены in-place!
	return true;
}
