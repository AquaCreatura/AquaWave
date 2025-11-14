#pragma once
#include "spg_defs.h"
#include "SpgRenderer.h"
#include "SpgScaler.h"

using  namespace fluctus;
using  namespace aqua_gui;

namespace spg_core
{


class SpgCore
{
public:
    SpgCore();
    ~SpgCore();
    bool Emplace            ();
    void Initialise         ( const freq_params& freq_params, const size_t samples_count = 1000);
    void SetTimeBounds      (const Limits<double>& power_bounds);
    void SetFreqBounds      (const Limits<double>& freq_bounds );
	void SetNfftOrder		(int fft_order);
    bool AccumulateNewData  (const std::vector<float>& passed_data, const double pos_ratio = 0.5);
    //Is Used to get pixmap from our spg
    QPixmap&         GetRelevantPixmap(const ChartScaleInfo& scale_info);
    spg_data const & GetSpectrogramInfo() const;
protected:
	void SetDataToColumn(const std::vector<float>& passed_data, Limits<size_t> row_id, size_t column_idx, spg_holder& holder_to_fill);
private:
    spg_data    spg_;
    SpgRenderer renderer_;
    SpgScaler   scaler_;


};

};