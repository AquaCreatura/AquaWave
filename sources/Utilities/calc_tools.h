#pragma once
#include <ipps.h>
#include <algorithm>
#include <vector>
#include <atomic>
#include <chrono>
#include <cstdint>

namespace utility_aqua
{
	
	template<typename T>
	T GetPercentile(const T* data, int data_size, double percentile, std::vector<T>& buff) {
		if (data_size <= 0) return T{};

		buff.resize(data_size);
		std::copy(data, data + data_size, buff.begin());

		// Вычисляем индекс с округлением до ближайшего целого
		int k = static_cast<int>(std::round(percentile / 100.0 * (data_size - 1)));

		std::nth_element(buff.begin(), buff.begin() + k, buff.end());
		return buff[k];
	}

	class DataSpeedEstimator {
	public:
		DataSpeedEstimator(const int test_interval_msec); //Задаём максимальный интервал в течении которого считаем
		void Process(const size_t data_size); //Обновляем наш счётчик (при превышении интервала - буфферезируем)
		void Reset(); //Сбрасываем текущие счётчики
		int  GetSamplesPerSec(); //Отдаём валидное значение
	protected:
		const int64_t interval_ms_;

		std::atomic<int64_t> current_{ 0 };   // данные текущего неполного интервала
		std::atomic<int64_t> last_{ 0 };      // данные за последний завершённый интервал
		std::atomic<int64_t> last_swap_time_ms_;

		static int64_t now_ms() {
			using namespace std::chrono;
			return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
		}
	};
}