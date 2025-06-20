#pragma once
#include "spg_defs.h"


using  namespace fluctus;
using  namespace aqua_gui;

namespace spg_core
{

class SpgScaler
{
    bool UpdateMinMax_X(const Limits<double>& new_bounds);
protected:
    spg_data &data_;  // Reference to the underlying data structure
};


};