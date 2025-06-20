#include "SpgRenderer.h"
#pragma once
using namespace spg_core;

spg_core::SpgRenderer::SpgRenderer(spg_data & init_val): sgp_(init_val)
{
}

QPixmap & spg_core::SpgRenderer::GetRelevantPixmap(const ChartScaleInfo & scale_info)
{
    // TODO: insert return statement here
}

bool spg_core::SpgRenderer::UpdateSpgRgbData()
{
    return false;
}

const argb_t * spg_core::SpgRenderer::GetNormalizedColor(double relative_density) const
{
    return nullptr;
}
