#pragma once
#include <ipps.h>
#include <stdint.h>
#include <qstring.h>
#include "Arks\Interfaces\ark_interface.h"
using namespace utility_aqua;
namespace analyzer
{

    struct AnalyzeDove: public fluctus::DoveParrent
    {
		AnalyzeDove() { base_thought = fluctus::DoveParrent::kSpecialThought; };
        enum SpectralThought : int64_t
        {
            kUnknown = 0, 
			kStartFromFileSource = 1,
        };
		fluctus::Limits<double> freq_bounds_hz; //In "Hz
		fluctus::Limits<double> file_bounds_ratio; //[0; 1]
    };

}