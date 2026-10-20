#include "PeakDetection.h"
#include <algorithm>
#include <cmath>
#include <limits>
using namespace peak_detection;
using namespace dpx_core;
std::vector<size_t> peak_detection::FindPeaksToAvg(const Ipp32f * data, size_t count_of_samples, int count_of_peaks, double check_area_ratio, double min_dist_ratio, double threshold)
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
	peaks.reserve(16);

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


		const double quality = double(data[i]) - average;

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
	constexpr double kPeakThresholdDb = 4.0;

	constexpr double kAcfCheckAreaRatio = 0.03;
	constexpr int    kMaxAcfHarmonic = 8;
	constexpr double kAcfToleranceRatio = 0.03;
	constexpr double kSymbolPeakRatio = 0.5;
}

PeakDetector::PeakDetector()
	: data_type_(kDpxChartType::kHarmonic),
	sample_rate_hz_(1.0),
	n_fft_(0),
	valid_(false)
{
}

void PeakDetector::Init(kDpxChartType data_type, int n_fft, double sample_rate_hz, double carrier_hz)
{
	Reset();
	max_peaks_count_ = 5;
	data_type_ = data_type;
	sample_rate_hz_ = sample_rate_hz;
	carrier_hz_ = carrier_hz;
	max_hold_.assign(n_fft, -std::numeric_limits<Ipp32f>::infinity());
	n_fft_ = n_fft;

	switch (data_type_)
	{

	case kDpxChartType::kACF:
		estim_params_.zero_shift = std::max<int>(n_fft_ /100, 2);
		estim_params_.norm_ratio = 1000. / sample_rate_hz;
		break;
	case kDpxChartType::kHarmonic:
		break;
	case kDpxChartType::kPower4x:
		estim_params_.add_after_norm = carrier_hz;
		estim_params_.norm_ratio = sample_rate_hz / n_fft_;
		estim_params_.add_before_norm = std::round( - 0.5 * n_fft_);
		break;
	case kDpxChartType::kEnvelope:
	case kDpxChartType::kPhasor:
		estim_params_.norm_ratio = sample_rate_hz / n_fft_ ;
		break;
	}
}

void PeakDetector::Reset()
{
	max_hold_.clear();
	n_fft_ = 0;
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
	if (n_fft_ != 0 && n_fft_ != count_of_samples)
	{
		Init(data_type_, n_fft_, sample_rate_hz_, carrier_hz_);
	}

	UpdateMaxHold(data, count_of_samples);

	valid_ = true;
}

void PeakDetector::UpdateMaxHold(const Ipp32f* data, size_t count_of_samples)
{
	if (n_fft_ != count_of_samples) {
		return;
		
	}

	for (size_t i = 0; i < count_of_samples; ++i)
	{
		// Утечка в dB: вычитаем фиксированную величину
		max_hold_[i] = std::max(
			data[i],
			max_hold_[i] - static_cast<Ipp32f>(kMaxHoldDecayDb));
	}
}

std::vector<double> PeakDetector::CalculatePeak() const
{
	if (!valid_ || n_fft_ < 3 || sample_rate_hz_ <= 0.0)
		return {};
	std::vector<size_t> peaks = FindPeaksToAvg( max_hold_.data() + estim_params_.zero_shift, n_fft_ - estim_params_.zero_shift, 1, kCheckAreaRatio, kMinDistRatio, kPeakThresholdDb);
	std::vector<double> double_peaks(peaks.size());
	for (int peak_counter = 0; peak_counter < peaks.size(); peak_counter++) {
		
		double_peaks[peak_counter] = (peaks[peak_counter] + estim_params_.zero_shift + estim_params_.add_before_norm) * estim_params_.norm_ratio + estim_params_.add_after_norm;
	}

	return double_peaks;
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
