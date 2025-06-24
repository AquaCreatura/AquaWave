#include "SpgCore.h"
#pragma once
using namespace spg_core;

spg_core::SpgScaler::SpgScaler(spg_data & passed_spg):
    data_(passed_spg)
{
}

bool spg_core::SpgScaler::UpdateMinMax_X(const Limits<double>& new_bounds)
{
    return false;
}
