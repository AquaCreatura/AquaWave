#include "DemodPipes.h"

pipes::CcmSyncer::CcmSyncer(const char * modulation, int upsample_koeff)
{
	ccm_man_.Init(modulation, upsample_koeff);
}

void pipes::CcmSyncer::ProcessData(PipeHolder::sptr meta_data)
{
	auto &passed = meta_data->complex_float_data;
	auto &synced = meta_data->buffer_32fc;
	std::swap(passed, synced);

	ccm_man_.GetSyncedIQ(passed, synced);
	if (next_) next_->ProcessData(meta_data);
}
