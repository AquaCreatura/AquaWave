#pragma once
#include <stdint.h>
//#include <ipps.h>
#include <vector>
#include <ippdefs.h>
namespace aqua_resampler
{
struct ResamplerSettings
{
    bool   need_norm_power  = true; //Should we normalise power after resample
	int	   denom_quality	= 10;
	bool   need_precise		= true;
	bool   skip_precise_fir	= false;
};

class ResamplerInterface
{
public:
    virtual ~ResamplerInterface() = default;
    virtual void SetSettings    (const ResamplerSettings settings                                                  ) = 0;
    virtual bool Init           (const int64_t base_fs_hz, int64_t& target_fs_hz, const int64_t bw_hz			   ) = 0;
    virtual bool ProcessData    (const Ipp32fc* passed_data, const size_t data_size, std::vector<Ipp32fc> &res_data) = 0;
	virtual bool ProcessData	(const std::vector<Ipp32fc> &passed_data, std::vector<Ipp32fc> &res_data		   ) = 0;
    virtual void Clear          ()                                                                                   = 0;
};


};