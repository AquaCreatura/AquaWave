#pragma once
#include <stdint.h>
//#include <ipps.h>
#include <vector>
#include <ippdefs.h>
namespace aqua_resampler
{
struct ResamplerSettings
{
    size_t filter_length    = 1024;  //length of the filter
    size_t max_denom        = 100;  //Relevant for MR filter
    bool   need_norm_power  = true; //Should we normalise power after resample
};

class ResamplerInterface
{
public:
    virtual ~ResamplerInterface() = default;
    virtual void SetSettings    (const ResamplerSettings settings                                                  ) = 0;
    virtual bool Init           (const int64_t base_samplerate   , int64_t& res_samplerate                         ) = 0;
    virtual bool ProcessData    (const Ipp32fc* passed_data, const size_t data_size, std::vector<Ipp32fc> &res_data) = 0;
    virtual void Clear          ()                                                                                   = 0;
};


};