#include "ContinuousDemodulator.h"
using namespace aq_demod;

ContinuousDemodulator::ContinuousDemodulator()
{
}

ContinuousDemodulator::~ContinuousDemodulator()
{
}

bool ContinuousDemodulator::Init(const char * modulation, int upsample_passed)
{
	return false;
}

bool ContinuousDemodulator::GetSyncedIQ(std::vector<Ipp32fc>& passed_iq, std::vector<Ipp32fc>& synced_iq)
{
	synced_iq.clear();
	ted_man_.Process(passed_iq, synced_iq);
	return true;
}
