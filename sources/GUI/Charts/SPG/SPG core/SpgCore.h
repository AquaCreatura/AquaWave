#pragma once
#include "spg_defs.h"


using  namespace fluctus;
using  namespace aqua_gui;

namespace spg_core
{


class SpgCore
{
public:
    SpgCore();
    ~SpgCore();
    void Initialise         ( const freq_params& freq_params, const size_t samples_count = 1000);
    void SetTimeBounds      (const Limits<double>& power_bounds);
    void SetFreqBounds      (const Limits<double>& freq_bounds );

    bool AccumulateNewData  (const std::vector<float>& passed_data, const double pos_ratio = 0.5);
    //Is Used to get pixmap from our dpx
    QPixmap&       GetRelevantPixmap(const ChartScaleInfo& scale_info);
private:
    spg_data spg_;

};

};