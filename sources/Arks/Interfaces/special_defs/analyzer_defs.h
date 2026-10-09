#pragma once
#include <ipps.h>
#include <stdint.h>
#include <qstring.h>
#include "Arks/Interfaces/ark_interface.h"
#include "Utilities/utility_aqua.h"
using namespace utility_aqua;
namespace analyzer
{
	enum class analz_chart_type : int {
		undefined = 0,
		kAcf = 1, //АКФ

		kPhasorSpectrum = 2, //Спектр мгновенной частоты (Символьная для PSK)
		kEnvelopeSpectrum = 4, //Спектр огибающей (Символьная для AM)

		kPowerSpectrum = 8, //Спектр степени (Отстройка несущей)

		kBandwidth = 16, //Ширина полосы

		kConstellation = 32
	};




    struct AnalyzeDove: public aqua::DoveParrent
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
		aqua::Limits<double> freq_bounds_hz; //In "Hz
		aqua::Limits<double> file_bounds_ratio; //[0; 1]
		aqua_opt<std::vector<float>> peaks;
		aqua_opt<std::string>	text_result;
		aqua_opt<double>		carrier_hz;
		aqua_opt<double>		symbol_rate_hz;
    };

}