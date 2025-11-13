#pragma once
#include <sstream>
#include <iomanip>
#include <regex>
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

#include <string>
#include <ctime>
#include <cstdio>

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

inline std::string ValueToString(double v, const int precision, const char* divider = ",") {

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

}