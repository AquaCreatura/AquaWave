#pragma once

#include <vector>
#include <cstddef>

#include <ipp.h>

class AcfWorker
{
public:
	AcfWorker() = default;
	~AcfWorker() = default;

	// ===== Pointer interface =====
	bool Process_32fc_32fc(const Ipp32fc* src, std::size_t size, Ipp32fc* dst, IppEnum acf_flags = ippAlgAuto | ippsNormA);

	bool Process_32fc_32f(const Ipp32fc* src, std::size_t size, Ipp32f* dst, IppEnum acf_flags = ippAlgAuto | ippsNormA);

	// ===== std::vector interface =====
	bool Process_32fc_32fc(const std::vector<Ipp32fc>& src, std::vector<Ipp32fc>& dst, IppEnum acf_flags = ippAlgAuto | ippsNormA);

	bool Process_32fc_32f(const std::vector<Ipp32fc>& src, std::vector<Ipp32f>& dst, IppEnum acf_flags = ippAlgAuto | ippsNormA);

private:
	bool UpdateBufferSize(int src_len, int dst_len, IppEnum acf_flags);

private:
	std::vector<Ipp8u> main_buff_;      // IPP working buffer
	std::vector<Ipp32fc> calc_buff_;    // temporary complex buffer (for magnitude path)
};
