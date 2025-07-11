#pragma once
#include <ipps.h>

namespace utility_aqua
{
    template<typename W>
    struct aqua_opt
    {
    public:
        const    W value() { return (val ? *val: W());  };
        operator W* () { return val; };
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