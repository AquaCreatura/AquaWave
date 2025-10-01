#pragma once
#ifndef AQUA_DEFINES_GUARD
#define AQUA_DEFINES_GUARD


#include <stdint.h>

namespace fluctus
{
    struct freq_params
    {
        int64_t samplerate{ 1'000 };
        int64_t carrier{ 0 };
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
}

#endif // !AQUA_DEFINES_GUARD