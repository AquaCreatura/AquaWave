#pragma once
#include <ipps.h>
#include <algorithm>
#include <vector>
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

}