#include "acf_worker.h"

bool AcfWorker::EnsureBufferSize(int src_len, int dst_len, IppEnum acf_flags)
{
	int min_buff_size = 0;
	if (ippsAutoCorrNormGetBufferSize(src_len, dst_len, ipp32fc, acf_flags, &min_buff_size) != ippStsNoErr)
		return false;

	main_buff_.resize(min_buff_size);
	return true;
}

// =======================================================
// Pointer interface
// =======================================================

bool AcfWorker::Process(const Ipp32fc* src, std::size_t size, Ipp32fc* dst, IppEnum acf_flags)
{
	if (!src || !dst || size == 0)
		return false;

	const int len = static_cast<int>(size);
	if (!EnsureBufferSize(len, len, acf_flags))
		return false;

	return ippsAutoCorrNorm_32fc(src, len, dst, len, acf_flags, main_buff_.data()) == ippStsNoErr;
}

bool AcfWorker::ProcessMagn(const Ipp32fc* src, std::size_t size, Ipp32f* dst, IppEnum acf_flags)
{
	if (!src || !dst || size == 0)
		return false;

	calc_buff_.resize(size);
	if (!Process(src, size, calc_buff_.data(), acf_flags))
		return false;

	return ippsMagnitude_32fc(calc_buff_.data(), dst, static_cast<int>(size)) == ippStsNoErr;
}

// =======================================================
// std::vector interface
// =======================================================

bool AcfWorker::Process(const std::vector<Ipp32fc>& src, std::vector<Ipp32fc>& dst, IppEnum acf_flags)
{
	if (src.empty())
		return false;

	dst.resize(src.size());
	return Process(src.data(), src.size(), dst.data(), acf_flags);
}

bool AcfWorker::ProcessMagn(const std::vector<Ipp32fc>& src, std::vector<Ipp32f>& dst, IppEnum acf_flags)
{
	if (src.empty())
		return false;

	dst.resize(src.size());
	return ProcessMagn(src.data(), src.size(), dst.data(), acf_flags);
}
