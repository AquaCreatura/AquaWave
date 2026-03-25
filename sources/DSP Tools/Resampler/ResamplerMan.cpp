#include "ResamplerMan.h"
#include "ResamplersImpl/MR Resampler/MultiRateResampler.h"
#include "ResamplersImpl/Precise Resampler/PreciseResampler.h"
#include <cmath>
#include <algorithm>

constexpr double max_error = 1.e-4;
using namespace aqua_resampler;
// Вспомогательная функция для расчёта длины FIR
static int get_fir_power_of_two(double resample_ratio, double util_factor) {
	if (resample_ratio <= 0 || util_factor >= 1.0 || util_factor <= 0) return 0;
	const double A = 60.0;
	double W = resample_ratio;
	double delta_f = W * (1.0 - util_factor);
	double N = (A - 8.0) / (14.36 * delta_f);
	double L = N + 1.0;
	return static_cast<int>(std::pow(2, std::ceil(std::log2(L))));
}

ResamplerManager::ResamplerManager() {
	// Настройки по умолчанию для MR ресемплера
	mr_settings_.filter_length = 512;
	mr_settings_.max_denom = 4;
	mr_settings_.filter_koeff = 0.95;

	// Настройки по умолчанию для Precise ресемплера
	precise_settings_.filter_length = 512;
	precise_settings_.filter_koeff = 0.95;
}

ResamplerManager::~ResamplerManager() {
	FreeResources();
}

bool ResamplerManager::initMRResampler(int64_t base_rate, int64_t& approx_target_rate) {
	double mr_ratio = static_cast<double>(approx_target_rate) / base_rate;
	// Корректируем длину фильтра в зависимости от коэффициента использования
	int fir_len = get_fir_power_of_two(mr_ratio, mr_settings_.filter_koeff);
	fir_len = qBound(16, fir_len, 1024);
	mr_settings_.filter_length = fir_len;

	mr_resampler_ = std::make_unique<MultiRateResampler>();
	mr_resampler_->SetSettings(mr_settings_);
	if (!mr_resampler_->Init(base_rate, approx_target_rate)) {
		mr_resampler_.reset();
		return false;
	}
	return true;
}

bool ResamplerManager::initPreciseResampler(int64_t base_rate, int64_t& target_rate) {

	double mr_ratio = static_cast<double>(target_rate) / base_rate;
	// Корректируем длину фильтра в зависимости от коэффициента использования
	int fir_len = get_fir_power_of_two(mr_ratio, precise_settings_.filter_koeff);
	fir_len = qBound(16, fir_len, 1024);
	precise_settings_.filter_length = fir_len;

	precise_resampler_ = std::make_unique<PreciseResampler>();

	precise_resampler_->SetSettings(precise_settings_);
	if (!precise_resampler_->Init(base_rate, target_rate)) {
		precise_resampler_.reset();
		return false;
	}
	return true;
}

bool ResamplerManager::Init(const fluctus::freq_params& base_params,
	fluctus::freq_params& target_params,
	bool precise) {
	FreeResources();

	int64_t base_rate = base_params.samplerate_hz;
	int64_t target_rate = target_params.samplerate_hz;

	if (base_rate <= 0 || target_rate <= 0) return false;

	freq_shifter_.Init(base_params.carrier_hz, target_params.carrier_hz, base_rate);
	resample_ratio_ = static_cast<double>(target_rate) / base_rate;
	int max_nom_denom = std::max(resample_ratio_, 1 / resample_ratio_) + 1;
	if (resample_ratio_ == 1.0) {
		target_params.samplerate_hz = base_rate;
		return true;
	}
	if(precise)
		mr_settings_.max_denom = 4 * max_nom_denom;
	else
		mr_settings_.max_denom = 5 * max_nom_denom;
	try {
		// 1. Подбираем рациональную дробь для MR ресемплера
		auto pq = FindBestFraction(resample_ratio_, mr_settings_.max_denom, true);
		int64_t approx_target_rate = base_rate * pq.first / pq.second;

		// 2. Инициализируем MR ресемплер
		if (!initMRResampler(base_rate, approx_target_rate))
			return false;

		// 3. Решаем, нужен ли Precise ресемплер
		bool need_precise = precise &&
			(std::abs(static_cast<double>(pq.first) / pq.second - resample_ratio_) > max_error);

		if (need_precise) {
			if (!initPreciseResampler(approx_target_rate, target_rate))
				return false;
			target_params.samplerate_hz = target_rate; // финальная частота
		}
		else {
			target_params.samplerate_hz = approx_target_rate;
		}

		return true;
	}
	catch (const std::exception&) {
		return false;
	}
}

bool ResamplerManager::ProcessBlock(const Ipp32fc* input_data, size_t size) {
	if (!input_data || size == 0) return false;

	shifted_data_.resize(size);
	freq_shifter_.ProcessBlock(input_data, shifted_data_.data(), size);

	if (resample_ratio_ == 1.0) {
		processed_data_.swap(shifted_data_);
		return true;
	}

	if (!mr_resampler_) return false;

	std::vector<Ipp32fc> mr_output;
	if (!mr_resampler_->ProcessData(shifted_data_.data(), shifted_data_.size(), mr_output))
		return false;

	if (precise_resampler_) {
		if (!precise_resampler_->ProcessData(mr_output.data(), mr_output.size(), processed_data_))
			return false;
	}
	else {
		processed_data_.swap(mr_output);
	}

	return true;
}

std::vector<Ipp32fc>& ResamplerManager::GetProcessedData() {
	return processed_data_;
}

void ResamplerManager::FreeResources() {
	if (mr_resampler_) {
		mr_resampler_->Clear();
		mr_resampler_.reset();
	}
	if (precise_resampler_) {
		precise_resampler_->Clear();
		precise_resampler_.reset();
	}
	processed_data_.clear();
	shifted_data_.clear();
}