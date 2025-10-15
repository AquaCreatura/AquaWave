#pragma once
#include <ipps.h>

namespace utility_aqua
{
    template<typename W>
    struct aqua_opt
    {
    public:
        const    W value() { return (val ? *val: W());  };
		const    W value_or(const W or_passed) { return (val ? *val : or_passed); };
        operator W* () { return val; };

		W* operator->()
		{
			return val;
		}
		const W* operator->() const
		{
			return val;
		}

        operator bool() const { return bool(val); }
        const W&  operator = (const W& passed_val)
        {
            if (val)
                (*val) = passed_val;
            else
                val = new W(passed_val);
            return *val;
        };
        void reset() { if (val) { delete val; val = nullptr; } };
    private:
        W* val = nullptr;
    };
};