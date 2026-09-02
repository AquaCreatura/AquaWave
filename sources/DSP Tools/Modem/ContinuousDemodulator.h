#pragma once
#include <ipps.h>
#include "TimePeakerDemod.h"
#include <string>
#include "TimePeakerDemod.h"
namespace aq_demod
{
class ContinuousDemodulator
{
public:
	ContinuousDemodulator();
	~ContinuousDemodulator();
	bool Init(const char* modulation, int upsample_passed);
	bool GetSyncedIQ(std::vector<Ipp32fc>& passed_iq, std::vector<Ipp32fc>& synced_iq);
private:
	GardnerTED ted_man_;
	//std::string modulation_name_:
};

}