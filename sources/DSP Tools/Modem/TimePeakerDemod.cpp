#include "TimePeakerDemod.h"
using namespace aq_demod;
TimePeakerDemod::TimePeakerDemod(int upsample_koeff)
{
}

TimePeakerDemod::~TimePeakerDemod()
{
}

bool TimePeakerDemod::Process(std::vector<Ipp32fc>& passed, std::vector<Ipp32fc>& out_samples)
{
	int res_size = passed.size() / upsample_koeff_;
	out_samples.resize(res_size);
	for (int i = 0; i < res_size; i++) {
		out_samples[i] = passed[i * 4];
	}
	return true;
}
