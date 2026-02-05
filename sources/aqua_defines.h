#pragma once
#ifndef AQUA_DEFINES_GUARD
#define AQUA_DEFINES_GUARD

#include <tbb/spin_mutex.h>
#include <vector>
#include <stdint.h>
#include <memory>
namespace fluctus
{
    struct freq_params
    {
		int64_t carrier_hz{ 0 };
        int64_t samplerate_hz{ 1'000 };
    };


    /** 
     * @brief Represents a numerical range with low and high values.
     * @tparam W Type of range values.
     */
    // Структура для хранения границ диапазона
    template<typename W = double>
    struct Limits {
        const bool operator==(const Limits<W>& right) const
        {
            return (low == right.low) && (high == right.high);
        }
        bool operator!=(const Limits<W>& right) const
        {
            return !((*this) == right);
        }
        W    delta() const 
        {
            return high - low;
        }
        W   mid () const
        {
            return (low + high) / 2;
        }

		Limits<W> operator*(const W& value) const
		{
			return { low * value, high * value };
		}

		// Оператор деления на скаляр
		Limits<W> operator/(const W& value) const
		{
			return { low / value, high / value };
		}
        W low;
        W high;
    };

	template <typename T>
	struct mutex_vec : std::vector<T> {
		tbb::spin_mutex mutex;
	};

	template <typename T>
	using shared_vec = std::shared_ptr<mutex_vec<T>>;
}

#endif // !AQUA_DEFINES_GUARD