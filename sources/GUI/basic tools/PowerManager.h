#pragma once

#include "aqua_defines.h"
#include <vector>
#include <atomic>
using namespace fluctus;
namespace aqua_gui
{

class PowerLimitMan
{
public:
    void SetNewViewBounds           (const Limits<double>& x_bounds);
    void SetPowerBounds             (const Limits<double>& power_bounds);
    Limits<double> GetPowerBounds   () const;
    void SetData                    (const std::vector<float>& data);
    bool IsAdaptiveMode             ();
protected:
    Limits<double> x_bounds_;
    bool           is_adaptive_mode_ = false;
    std::atomic<Limits<double>> power_bounds_;
};



}