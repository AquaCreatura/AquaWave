#pragma once
#include <vector>
#include <ipps.h>
namespace aq_demod
{
class TimePeakerDemod
{
public:
	TimePeakerDemod(int upsample_koeff = 4);
	~TimePeakerDemod();
	bool Process(std::vector<Ipp32fc> &passed, std::vector<Ipp32fc> &out_samples);
private:
	int upsample_koeff_ = 4;
};
}