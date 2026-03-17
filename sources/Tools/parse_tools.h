#pragma once

#include <fstream>
#include <cstdint>

#include <string>
#include <ctime>
#include <cstdio>

#include <sstream>
#include <iomanip>
#include <regex>

#include "utility_aqua.h"

namespace aqua_parse_tools
{
//AMOS 5_17E_LinkWay-S2_1137.01MHz 23312.5KHz.pcm - Why can not parse this string?
inline bool get_samplerate_from_filename(const std::string& filename, int64_t &samplerate_hz)
{
    // Разрешаем любой регистр единиц, дробную часть — опционально,
    // и возможный пробел перед 'kHz'
    static const std::regex symbol_rate_regex(
        R"((\d+(?:\.\d+)?)\s*kHz)",
        std::regex_constants::icase
    );
    std::smatch match;
    if (std::regex_search(filename, match, symbol_rate_regex) && match.size() > 1) 
    {
        double khz = std::stod(match[1].str());
        samplerate_hz = static_cast<int64_t>(khz * 1e3);
        return true;
    }
    return false;
}

inline bool get_carrier_from_filename(const std::string& filename, int64_t &carrier_hz)
{
    static const std::regex carrier_regex(
        R"((\d+(?:\.\d+)?)\s*MHz)",
        std::regex_constants::icase
    );
    std::smatch match;
    if (std::regex_search(filename, match, carrier_regex) && match.size() > 1) {
        double mhz = std::stod(match[1].str());
        carrier_hz = static_cast<int64_t>(mhz * 1e6);
        return true;
    }
    return false;
}


inline void parse_filename(const std::string& filename, int64_t &carrier_hz, int64_t &samplerate_hz) 
{
    get_samplerate_from_filename(filename, samplerate_hz);
    get_carrier_from_filename(filename, carrier_hz);
}

inline std::string generate_filename(const int64_t carrier_hz,
                              const int64_t symbol_rate_hz,
                              const std::string& comment,
                              const std::string& extension = "pcm") 
{
    std::ostringstream oss;
    oss << comment;
    {
        oss << ' ' << std::fixed << std::setprecision(6) 
            << (carrier_hz / 1.e6) << "MHz";
    }
    {
        oss << ' ' << std::fixed << std::setprecision(3) 
            << (symbol_rate_hz / 1.e3) << "KHz";
    }
    oss << '.' << extension;
    return oss.str();
}



// Функция генерирует имя файла с датой, временем и комментариями
// Формат: "dd.mm.yyyy hh_mm_ss comment"
inline std::string gen_time_string(const std::string& comment = "") {
    // Получаем текущее время
    std::time_t t = std::time(nullptr);

    // Преобразуем время в локальное представление
    struct tm local_tm;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&local_tm, &t);  // Безопасная версия для Windows
#else
    localtime_r(&t, &local_tm);  // Потокобезопасная версия для POSIX-систем
#endif

    // Буфер для формирования строки имени файла
    char buffer[64] = {0};
    // Форматирование: dd.mm.yyyy hh_mm_ss comment
    // Обратите внимание: local_tm.tm_mon хранит месяц от 0 до 11, tm_year – количество лет с 1900
    std::snprintf(buffer, sizeof(buffer), "%02d.%02d.%04d %02d_%02d_%02d %s",
                  local_tm.tm_mday, local_tm.tm_mon + 1, local_tm.tm_year + 1900,
                  local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec,
                  comment.c_str());
    
    return std::string(buffer);
}

inline double GetPrecission(double value) {
	return std::max(0., -1 * log10(value) + 3);
}

inline std::string ValueToString(double v, int precision, const char* divider = ",") {

	if (precision < 0) precision = GetPrecission(v);
	bool neg = v < 0;
	if (neg) v = -v;
	long long int_part = static_cast<long long>(v);

	double frac = v - int_part;
	// --- Форматируем целую часть ---
	std::string s = std::to_string(int_part);
	int n = static_cast<int>(s.size());
	for (int i = n - 3; i > 0; i -= 3)
		s.insert(i, divider);
	// --- Форматируем дробную часть ---
	if (precision > 0) {
		s += '.';
		double scale = std::pow(10, precision);
		long long frac_part = static_cast<long long>(frac * scale + 0.5);
		auto frac_str = std::to_string(frac_part);
		if (frac_str.size() < static_cast<size_t>(precision))
			frac_str.insert(0, precision - frac_str.size(), '0');
		s += frac_str;
	}
	if (neg) s.insert(0, "-");
	return s;
}



using namespace utility_aqua;
struct WavInfo
{
	aqua_opt<uint16_t> audio_format;
	aqua_opt<uint16_t> num_channels;
	aqua_opt<uint32_t> sample_rate;
	aqua_opt<uint32_t> byte_rate;
	aqua_opt<uint16_t> block_align;
	aqua_opt<uint16_t> bits_per_sample;

	aqua_opt<uint32_t> data_offset;
	aqua_opt<uint32_t> data_size;
};



inline bool get_wav_info(const std::string& filename, WavInfo& out)
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


}