#include "calc_tools.h"

utility_aqua::DataSpeedEstimator::DataSpeedEstimator(const int test_interval_msec) :
	interval_ms_(test_interval_msec), last_swap_time_ms_(now_ms())
{
}

void utility_aqua::DataSpeedEstimator::Process(size_t data_size)
{
	current_ += static_cast<int64_t>(data_size);

	const auto now = now_ms();
	if (now - last_swap_time_ms_ < interval_ms_) {
		return;
	}

	last_swap_time_ms_ = now;
	last_.store(std::max(last_, current_));
	current_ = 0;
}

void utility_aqua::DataSpeedEstimator::Reset()
{
	current_.store(0, std::memory_order_relaxed);
	last_.store(0, std::memory_order_relaxed);
	last_swap_time_ms_.store(now_ms(), std::memory_order_relaxed);
}

int utility_aqua::DataSpeedEstimator::GetSamplesPerSec()
{
	const int64_t last = last_.load(std::memory_order_acquire);
	if (last != 0) {
		return static_cast<int>(last * 1000 / interval_ms_);
	}
	return 0;
	const int64_t elapsed = now_ms() - last_swap_time_ms_.load(std::memory_order_acquire);
	if (elapsed <= 0) {
		return 0;
	}

	const int64_t cur = current_.load(std::memory_order_acquire);
	return static_cast<int>(cur * 1000 / elapsed);
}

