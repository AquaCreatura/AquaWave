#pragma once
#include <ipps.h>
#include <stdint.h>
#include <qstring.h>
#include "Arks/Interfaces/ark_interface.h"
using namespace utility_aqua;
namespace file_writer
{

    struct FileWriterDove: public fluctus::DoveParrent
    {
		FileWriterDove(thoughts_list thooghts) { special_thought = thooghts; };
		FileWriterDove() = default;
        enum SpecThought : int64_t
        {
            kUnknown = 0, 
			kRecordSelection = 1,
        };
		fluctus::Limits<double> freq_bounds_hz; //In "Hz
		fluctus::Limits<double> file_bounds_ratio; //[0; 1]
    };

}