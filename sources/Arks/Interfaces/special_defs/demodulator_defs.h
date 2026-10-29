#pragma once
#include "Arks/Interfaces/ark_interface.h"

#include <stdint.h>

using namespace utility_aqua;
namespace demodulator
{
    struct DemodulatorDove: public aqua::DoveParrent
    {
		DemodulatorDove(thoughts_list thooghts) { special_thought = thooghts; };
		DemodulatorDove() {};
        enum SpecThought : int64_t
        {
            kUnknown = 0,
            kInitialise = (1 << 0)
        };
		aqua_opt<double>				carrier_hz;
		aqua_opt<double>				symbol_rate_hz;
    };

}
