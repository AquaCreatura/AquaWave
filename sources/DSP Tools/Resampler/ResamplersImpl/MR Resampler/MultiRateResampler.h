
#pragma once
#include <ipps.h>
#include <memory>
#include "..\ResamperInterface.h"
#include "..\..\tools\resampler_tools.h"
namespace aqua_resampler

{

class MultiRateResampler : public ResamplerInterface {
public:
    MultiRateResampler();
    virtual ~MultiRateResampler() override;

    virtual void SetSettings    (const ResamplerSettings settings                                                  ) override;
    virtual bool Init           (const int64_t base_samplerate, int64_t& res_samplerate                            ) override;
    virtual bool ProcessData    (const Ipp32fc* passed_data, const size_t data_size, std::vector<Ipp32fc>& res_data) override;
    virtual void Clear          () override;

private:
    ResamplerSettings       settings_;
    IppsFIRSpec_32fc* pSpec_;
    std::vector<Ipp8u> spec_buffer_;
    std::vector<Ipp8u> work_buffer_;
    std::vector<Ipp32fc> delay_line_;
    int up_factor_;
    int down_factor_;
    int64_t base_rate_;

};


}
