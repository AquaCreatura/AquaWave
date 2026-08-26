#include "PeakDetection.h"
#include <algorithm>
#include <cmath>
#include <limits>
using namespace peak_detection;
using namespace dpx_core;
std::vector<size_t> peak_detection::FindPeaksToAvg(Ipp32f * data, size_t count_of_samples, int count_of_peaks, double check_area_ratio, double min_dist_ratio, double threshold)
{
	if (data == nullptr || count_of_samples < 3 || count_of_peaks <= 0)
		return {};

	const size_t check_radius = std::max<size_t>(1, static_cast<size_t>(count_of_samples * check_area_ratio * 0.5));
	const size_t min_distance = std::max<size_t>(1, static_cast<size_t>(count_of_samples * min_dist_ratio));

	struct Peak
	{
		size_t index;
		double quality;
	};

	std::vector<Peak> peaks;
	peaks.reserve(count_of_samples / 4);

	// Ищем локальные максимумы.
	for (size_t i = 1; i + 1 < count_of_samples; ++i)
	{
		if (data[i] < data[i - 1] || data[i] < data[i + 1])
			continue;

		const size_t left = (i > check_radius) ? i - check_radius : 0;
		const size_t right = std::min(count_of_samples - 1, i + check_radius);

		Ipp32f left_mean = 0.0f;
		Ipp32f right_mean = 0.0f;

		const int left_count = static_cast<int>(i - left);
		const int right_count = static_cast<int>(right - i);

		if (left_count > 0)
			ippsMean_32f(data + left, left_count, &left_mean, ippAlgHintFast);

		if (right_count > 0)
			ippsMean_32f(data + i + 1, right_count, &right_mean, ippAlgHintFast);

		double average = 0.0;

		if (left_count > 0 && right_count > 0)
			average = (double(left_mean) * left_count + double(right_mean) * right_count) / double(left_count + right_count);
		else if (left_count > 0)
			average = left_mean;
		else if (right_count > 0)
			average = right_mean;

		if (average <= 0.0)
			continue;

		const double quality = double(data[i]) / average;

		if (quality >= threshold)
			peaks.push_back({ i, quality });
	}

	// Лучшие пики идут первыми.
	std::sort(peaks.begin(), peaks.end(), [](const Peak& a, const Peak& b)
	{
		return a.quality > b.quality;
	});

	std::vector<size_t> result;
	result.reserve(std::min<size_t>(count_of_peaks, peaks.size()));

	// Greedy selection с минимальным расстоянием.
	for (const Peak& peak : peaks)
	{
		bool too_close = false;

		for (const size_t selected : result)
		{
			if (std::abs(int64_t(peak.index) - int64_t(selected)) < int64_t(min_distance))
			{
				too_close = true;
				break;
			}
		}

		if (!too_close)
		{
			result.push_back(peak.index);

			if (result.size() >= static_cast<size_t>(count_of_peaks))
				break;
		}
	}

	std::sort(result.begin(), result.end());

	return result;
}



namespace
{
	constexpr double kMaxHoldDecayDb = 0.05;
	constexpr double kCheckAreaRatio = 0.03;
	constexpr double kMinDistRatio = 0.005;
	constexpr double kPeakThresholdDb = 2.0;

	constexpr double kAcfCheckAreaRatio = 0.03;
	constexpr int    kMaxAcfHarmonic = 8;
	constexpr double kAcfToleranceRatio = 0.03;
	constexpr double kSymbolPeakRatio = 0.5;
}

PeakDetector::PeakDetector()
	: data_type_(kDpxChartType::kHarmonic),
	sample_rate_hz_(1.0),
	count_of_samples_(0),
	valid_(false)
{
}

void PeakDetector::Init(kDpxChartType data_type, double sample_rate_hz, double carrier_hz)
{
	max_peaks_count_ = 5;

	data_type_ = data_type;
	sample_rate_hz_ = sample_rate_hz;
	carrier_hz_ = carrier_hz;
	Reset();
}

void PeakDetector::Reset()
{
	max_hold_.clear();
	count_of_samples_ = 0;
	valid_ = false;
}

void PeakDetector::Process(const Ipp32f* data, size_t count_of_samples)
{
	if (!data || count_of_samples == 0)
	{
		valid_ = false;
		return;
	}

	// Если изменился размер, полностью сбрасываем состояние
	if (count_of_samples_ != 0 && count_of_samples_ != count_of_samples)
	{
		Reset();
	}

	count_of_samples_ = count_of_samples;

	UpdateMaxHold(data, count_of_samples);

	valid_ = true;
}

void PeakDetector::UpdateMaxHold(const Ipp32f* data, size_t count_of_samples)
{
	if (n_fft_ != count_of_samples) {
		max_hold_.assign(count_of_samples, -std::numeric_limits<Ipp32f>::infinity());
		n_fft_ = count_of_samples;
	}

	for (size_t i = 0; i < count_of_samples; ++i)
	{
		// Утечка в dB: вычитаем фиксированную величину
		max_hold_[i] = std::max(
			data[i],
			max_hold_[i] - static_cast<Ipp32f>(kMaxHoldDecayDb));
	}
}

double PeakDetector::GetPeak() const
{
	if (!valid_ || n_fft_ < 3 || sample_rate_hz_ <= 0.0)
		return std::numeric_limits<double>::quiet_NaN();

	switch (data_type_)
	{
	case kDpxChartType::kACF:
		return GetPeakAcf();
	case kDpxChartType::kHarmonic:
		return GetPeakSpectrumMax();
	case kDpxChartType::kPower4x:
		return GetPeakSpectrumSymmetry();
	case kDpxChartType::kEnvelope:
	case kDpxChartType::kPhasor:
		return GetPeakSymbolRate();
	default:
		return std::numeric_limits<double>::quiet_NaN();
	}
}

double PeakDetector::GetPeakSpectrumMax() const
{
	std::vector<size_t> peaks = FindPeaksToAvg(
		const_cast<Ipp32f*>(max_hold_.data()),
		n_fft_,
		max_peaks_count_,
		kCheckAreaRatio,
		kMinDistRatio,
		kPeakThresholdDb);

	if (peaks.empty())
		return std::numeric_limits<double>::quiet_NaN();

	size_t best_peak = *std::max_element(
		peaks.begin(), peaks.end(),
		[this](size_t a, size_t b) { return max_hold_[a] < max_hold_[b]; });

	double freq_hz = GetInterpolatedPeak(best_peak, max_hold_) * sample_rate_hz_ / n_fft_;
	return freq_hz;
}

double PeakDetector::GetPeakSpectrumSymmetry() const
{
	std::vector<size_t> peaks = FindPeaksToAvg(
		const_cast<Ipp32f*>(max_hold_.data()),
		n_fft_,
		max_peaks_count_,
		kCheckAreaRatio,
		kMinDistRatio,
		kPeakThresholdDb);

	if (peaks.empty())
		return std::numeric_limits<double>::quiet_NaN();

	std::vector<size_t> sorted_peaks = peaks;
	std::sort(sorted_peaks.begin(), sorted_peaks.end());

	double sum = 0.0;
	for (size_t p : sorted_peaks)
		sum += GetInterpolatedPeak(p, max_hold_);

	double bin_delta = sum / sorted_peaks.size()- n_fft_ / 2;


	double freq_hz = carrier_hz_ + bin_delta * sample_rate_hz_ / n_fft_ / 4;
	return freq_hz;
}

double PeakDetector::GetPeakAcf() const
{
	std::vector<size_t> peaks = FindPeaksToAvg(
		const_cast<Ipp32f*>(max_hold_.data()),
		n_fft_,
		max_peaks_count_,
		kAcfCheckAreaRatio,
		kMinDistRatio,
		kPeakThresholdDb);

	if (peaks.empty())
		return std::numeric_limits<double>::quiet_NaN();

	// Отбрасываем нулевой лаг
	std::vector<size_t> lags;
	for (size_t p : peaks)
		if (p != 0) lags.push_back(p);
	if (lags.empty())
		return std::numeric_limits<double>::quiet_NaN();

	size_t fundamental = FindFundamental(lags, kAcfToleranceRatio, kMaxAcfHarmonic);
	if (fundamental == 0)
		fundamental = lags.front();

	double period_ms = (static_cast<double>(fundamental) / sample_rate_hz_) * 1000.0;
	return period_ms;
}

double PeakDetector::GetPeakSymbolRate() const
{
	std::vector<size_t> peaks = FindPeaksToAvg(
		const_cast<Ipp32f*>(max_hold_.data()),
		n_fft_,
		max_peaks_count_,
		kCheckAreaRatio,
		kMinDistRatio,
		kPeakThresholdDb);

	if (peaks.empty())
		return std::numeric_limits<double>::quiet_NaN();

	size_t fundamental = FindFundamental(peaks, kAcfToleranceRatio, kMaxAcfHarmonic);

	if (fundamental != 0)
	{
		auto max_amp_iter = std::max_element(
			peaks.begin(), peaks.end(),
			[this](size_t a, size_t b) { return max_hold_[a] < max_hold_[b]; });
		double max_amp = max_hold_[*max_amp_iter];
		double fund_amp = max_hold_[fundamental];

		if (fund_amp < kSymbolPeakRatio * max_amp)
			fundamental = *max_amp_iter;
	}
	else
	{
		auto max_amp_iter = std::max_element(
			peaks.begin(), peaks.end(),
			[this](size_t a, size_t b) { return max_hold_[a] < max_hold_[b]; });
		fundamental = *max_amp_iter;
	}

	double freq_hz = GetInterpolatedPeak(fundamental, max_hold_) * sample_rate_hz_ / n_fft_;
	return freq_hz;
}

double PeakDetector::GetInterpolatedPeak(size_t peak_index, const std::vector<Ipp32f>& data) const
{
	if (peak_index == 0 || peak_index >= data.size() - 1)
		return static_cast<double>(peak_index);

	double alpha = data[peak_index - 1];
	double beta = data[peak_index];
	double gamma = data[peak_index + 1];

	double denom = alpha - 2.0 * beta + gamma;
	if (std::abs(denom) < 1e-12)
		return static_cast<double>(peak_index);

	double delta = 0.5 * (alpha - gamma) / denom;
	delta = std::max(-0.5, std::min(0.5, delta));
	return static_cast<double>(peak_index) + delta;
}

size_t PeakDetector::FindFundamental(const std::vector<size_t>& peaks,
	double tolerance_ratio,
	int max_harmonic) const
{
	if (peaks.empty())
		return 0;

	std::vector<size_t> sorted = peaks;
	std::sort(sorted.begin(), sorted.end());
	sorted.erase(std::remove(sorted.begin(), sorted.end(), 0), sorted.end());

	if (sorted.empty())
		return 0;
	if (sorted.size() == 1)
		return sorted.front();

	size_t best_fund = 0;
	size_t best_support = 0;

	for (size_t candidate : sorted)
	{
		size_t support = 1;
		for (size_t other : sorted)
		{
			if (other == candidate)
				continue;
			if (IsHarmonicOf(other, candidate, tolerance_ratio, max_harmonic))
				++support;
		}

		if (support > best_support ||
			(support == best_support && candidate < best_fund))
		{
			best_support = support;
			best_fund = candidate;
		}
	}

	if (best_support >= 2)
		return best_fund;

	return sorted.front();
}

bool PeakDetector::IsHarmonicOf(size_t p, size_t f, double tolerance, int max_harmonic) const
{
	if (f == 0 || p == 0)
		return false;

	double ratio = static_cast<double>(p) / static_cast<double>(f);
	int nearest = static_cast<int>(std::round(ratio));
	if (nearest < 1 || nearest > max_harmonic)
		return false;

	double expected = nearest * f;
	double error = std::abs(static_cast<double>(p) - expected) / static_cast<double>(p);
	return error <= tolerance;
}