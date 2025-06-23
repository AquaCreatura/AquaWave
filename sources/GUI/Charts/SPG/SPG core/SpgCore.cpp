#include "SpgCore.h"
#pragma once
using namespace spg_core;

spg_core::SpgCore::SpgCore(): renderer_(spg_), scaler_(spg_)
{
}

void spg_core::SpgCore::SetTimeBounds(const Limits<double>& power_bounds)
{
}


void spg_core::SpgCore::SetFreqBounds(const Limits<double>& freq_bounds)
{
}

bool spg_core::SpgCore::AccumulateNewData(const std::vector<float>& passed_data, const double pos_ratio)
{
    return false;
}

QPixmap & spg_core::SpgCore::GetRelevantPixmap(const ChartScaleInfo & scale_info)
{
    // TODO: insert return statement here
}

spg_core::spg_data const & spg_core::SpgCore::GetSpectrogramInfo() const
{
    return spg_;
}

spg_core::SpgCore::~SpgCore()
{
}

void spg_core::SpgCore::Initialise(const freq_params & freq_params, const size_t samples_count)
{
}
