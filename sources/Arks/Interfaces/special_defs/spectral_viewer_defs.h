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
			kSetSelectionHolder = 2,

        };
        aqua_opt<int> fft_order_;
		aqua_opt<std::shared_ptr<aqua_gui::SelectionHolder>> sel_holder;
    };

}