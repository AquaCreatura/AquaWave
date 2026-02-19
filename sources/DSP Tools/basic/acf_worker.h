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
	bool Process(const Ipp32fc* src,
		std::size_t size,
		Ipp32fc* dst,
		IppEnum acf_flags = ippAlgAuto | ippsNormB);

	bool ProcessMagn(const Ipp32fc* src,
		std::size_t size,
		Ipp32f* dst,
		IppEnum acf_flags = ippAlgAuto | ippsNormB);

	// ===== std::vector interface =====
	bool Process(const std::vector<Ipp32fc>& src,
		std::vector<Ipp32fc>& dst,
		IppEnum acf_flags = ippAlgAuto | ippsNormB);

	bool ProcessMagn(const std::vector<Ipp32fc>& src,
		std::vector<Ipp32f>& dst,
		IppEnum acf_flags = ippAlgAuto | ippsNormB);

private:
	bool EnsureBufferSize(int src_len, int dst_len, IppEnum acf_flags);

private:
	std::vector<Ipp8u> main_buff_;      // IPP working buffer
	std::vector<Ipp32fc> calc_buff_;    // temporary complex buffer (for magnitude path)
};
