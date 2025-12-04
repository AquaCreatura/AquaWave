#pragma once
#include <ipps.h>
#include <stdint.h>
#include <qstring.h>
#include "Arks\Interfaces\ark_interface.h"
using namespace utility_aqua;
namespace spectral_viewer
{

    struct SpectralDove : public fluctus::DoveParrent
    {
		SpectralDove() { base_thought = fluctus::DoveParrent::kSpecialThought; };
        enum SpectralThought : int64_t
        {
            kUnknown = 0, 
            kSetFFtOrder = 1,
        };
        aqua_opt<int> fft_order_;
    };

}