#include "SpgRequester.h"
#include <ippvm.h>
using namespace spg_core; // Используем пространство имён dpx_core

spg_core::SpgRequester::SpgRequester(const spg_data & spg) : spg_(spg)
{
}

void spg_core::SpgRequester::RequestData()
{
}


void spg_core::SpgRequester::SetFileSource(const ArkWptr & file_source)
{
    ark_file_src_ = file_source;
}

void spg_core::SpgRequester::SetArkSpectrogram(const ArkWptr & ark_spg)
{
    ark_spg_ = ark_spg;
}
