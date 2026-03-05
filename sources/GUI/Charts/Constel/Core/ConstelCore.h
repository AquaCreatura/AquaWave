#pragma once
#include <ipps.h>
#include "constel_defs.h"
namespace constel {


class ConstelCore {
public:
	ConstelCore();

	void AddData(const std::vector<Ipp32fc> &passed_data);
	constellation_data& GetConstelData();
protected:
	constellation_data data_;
};

}