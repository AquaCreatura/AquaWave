#pragma once
#include <qpointer.h>
#include "Interfaces/base_impl/ark_base.h"
#include "helpers/SpgRequester.h"
#include "GUI/Charts/SPG/SpgChart.h"
#include "DSP Tools/FFT/FFT_Worker.h"
#include "window/SpgWindow.h"
namespace spg_core
{

class Spectrogram : public fluctus::ArkBase
{

public:
    Spectrogram(QWidget *parrent = nullptr);
    ~Spectrogram();
    virtual bool SendData(fluctus::DataInfo const& data_info) override;
    virtual bool SendDove(fluctus::DoveSptr const & sent_dove) override;
    ArkType GetArkType() const override;
protected:
    bool Reload();
protected:
    QPointer<ChartSPG>          spg_drawer_;
    std::shared_ptr<SpgWindow>  window_;
    WorkBounds                  time_bounds_;
    FFT_Worker                  fft_worker_;
    SpgRequester                requester_;
};



}