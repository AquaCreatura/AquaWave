#include "DemodPipes.h"

pipes::CcmSyncer::CcmSyncer(const char * modulation, int upsample_koeff)
{
	ccm_man_.Init(modulation, upsample_koeff);
}

void pipes::CcmSyncer::ProcessData(PipeHolder::sptr meta_data)
{
	auto &passed = meta_data->complex_float_data;
	auto &synced = meta_data->buffer_32fc;
	ccm_man_.SynchroniseIQ(passed, synced);
	std::swap(passed, synced);
	if (next_) next_->ProcessData(meta_data);
}
