#pragma once
#include <ipps.h>
#include "constel_defs.h"
#include "ConstelRenderer.h"
namespace constel {


class ConstelCore {
public:
	ConstelCore();

	void				AddData(const std::vector<Ipp32fc> &passed_data);
	constellation_data& GetConstelData();
	void				Emplace(const int bins_amplitude = 128);
	QPixmap&			GetRelevantPixmap(const int chart_size_px);
protected:
	void CheckPassedMaximum(const std::vector<Ipp32fc>& data);
	void SetNewMaximum(const Ipp32f max_value);
	void StoreData(const std::vector<Ipp32fc>& data);
protected:
	constellation_data constel_;
	ConstellRenderer renderer_;
};

}