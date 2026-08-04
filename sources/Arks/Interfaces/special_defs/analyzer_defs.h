#pragma once
#include <ipps.h>
#include <stdint.h>
#include <qstring.h>
#include "Arks/Interfaces/ark_interface.h"
#include "Utilities/utility_aqua.h"
using namespace utility_aqua;
namespace analyzer
{

    struct AnalyzeDove: public fluctus::DoveParrent
    {
		AnalyzeDove(thoughts_list thooghts) { special_thought = thooghts; };
		AnalyzeDove() {};
        enum SpecThought : int64_t
        {
            kUnknown = 0, 
			kStartFromFileSource = 1,
			kGetHarmonicResult	 = 2,
			kSetHarmonicInfo	 = 3
        };
		fluctus::Limits<double> freq_bounds_hz; //In "Hz
		fluctus::Limits<double> file_bounds_ratio; //[0; 1]
		aqua_opt<double>		peak_value;
		aqua_opt<std::string>	text_result;
		aqua_opt<double>		carrier_hz;
		aqua_opt<double>		symbol_rate_hz;
    };

}