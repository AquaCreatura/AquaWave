#pragma once
#include "BasePipes.h"
#include "DSP Tools/Modem/ContinuousDemodulator.h"
namespace pipes {

class CcmSyncer : public PipeInterface
{
public:
	CcmSyncer(const char * modulation, int upsample_koeff = 4);
	void ProcessData(PipeHolder::sptr meta_data) override;
protected:
	aq_demod::ContinuousDemodulator ccm_man_;
};



}