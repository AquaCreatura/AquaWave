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
	pivots_ = CalculateModulationPivots(modulation);
	return false;
}

Ipp32fc aq_demod::ContinuousDemodulator::GetClosestSymbol(Ipp32fc passed)
{
	if (pivots_.empty()) return Ipp32fc();
	Ipp32fc best_symbol = pivots_[0];
	double least_euclid = 1.e30;
	for (auto piv_iter : pivots_) {
		double y_delta = (piv_iter.im - passed.im);
		double x_delta = (piv_iter.re - passed.re);
		double cur_euclid = y_delta * y_delta + x_delta * x_delta;
		if (cur_euclid < least_euclid) {
			best_symbol = piv_iter;
			cur_euclid  = least_euclid;
		}
	}

}

bool ContinuousDemodulator::SynchroniseIQ(const std::vector<Ipp32fc>& passed_iq, std::vector<Ipp32fc>& synced_iq)
{
	synced_iq.clear();
	ted_man_.Process(passed_iq, synced_iq);
	f0_;
	ph0_;
	for (Ipp32fc &sample : synced_iq) {
		

	}

	return true;
}


