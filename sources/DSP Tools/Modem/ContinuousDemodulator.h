#pragma once
#include <ipps.h>
#include "TimePeakerDemod.h"
#include <string>
#include "TimePeakerDemod.h"
#include "ModulationPoints.h"
namespace aq_demod
{
class ContinuousDemodulator
{
public:
	ContinuousDemodulator();
	~ContinuousDemodulator();
	bool Init(const char* modulation, int upsample_passed);
	bool SynchroniseIQ(const std::vector<Ipp32fc>& passed_iq, std::vector<Ipp32fc>& synced_iq);
protected:
	Ipp32fc GetClosestSymbol(Ipp32fc passed);
private:
	GardnerTED ted_man_;
	std::vector<Ipp32fc> pivots_;
	std::vector<Ipp32fc> res_iq_;

	const double f0_ {0.}; //Текущая частотная отстройка
	const double ph0_{ 0. }; //Текущая фазовая отстройка

	//std::string modulation_name_:
};

}