#include "ResamplerMan.h"
#include "ResamplersImpl/MR Resampler/MultiRateResampler.h"
#include "ResamplersImpl/Precise Resampler/PreciseResampler.h"
#include <cmath>
#include <algorithm>

constexpr double max_error = 1.e-4;
using namespace aqua_resampler;

ResamplerManager::ResamplerManager() {
	settings_.denom_quality = 10; // Настройки по умолчанию для MR
	settings_.need_precise = true;
	settings_.skip_precise_fir = true;
	mr_resampler_		= std::make_unique<MultiRateResampler>();
	precise_resampler_	= std::make_unique<PreciseResampler>();
	SetSettings(settings_);
}

ResamplerManager::~ResamplerManager() {
	FreeResources();
}

bool aqua_resampler::ResamplerManager::SetBaseParams(const int64_t carrier_hz, const int64_t samplerate_hz)
{
	if (samplerate_hz < 0) return false;
	base_params_.carrier_hz		= carrier_hz;
	base_params_.samplerate_hz	= samplerate_hz;
	return true;
}

bool aqua_resampler::ResamplerManager::SetTargetParams(const int64_t fc_tgt_hz, int64_t& sr_tgt_hz, const int64_t target_bw_hz)
{
	FreeResources();

	if (sr_tgt_hz <= 0) return false;
	

	freq_shifter_.Init(base_params_.carrier_hz, fc_tgt_hz, base_params_.samplerate_hz);

	resample_ratio_ = static_cast<double>(sr_tgt_hz) / base_params_.samplerate_hz;

	if (resample_ratio_ == 1.0) { //Передискретизация не требуется
		return true;
	}

	int64_t sr_after_mr = sr_tgt_hz;
	if (!mr_resampler_->Init(base_params_.samplerate_hz, sr_after_mr, target_bw_hz)) 
		return false;

	if (settings_.need_precise) {
		if (!precise_resampler_->Init(sr_after_mr, sr_tgt_hz, target_bw_hz))
			return false;
	}
	else
	{
		sr_tgt_hz = sr_after_mr;
	}
	

	return true;
}


bool ResamplerManager::ProcessBlock(const Ipp32fc* input_data, size_t size) {
	if (!input_data || size == 0) return false;

	shifted_data_.resize(size);
	freq_shifter_.ProcessBlock(input_data, shifted_data_.data(), size);

	if (resample_ratio_ == 1.0) {
		processed_data_.swap(shifted_data_);
		return true;
	}

	if (!mr_resampler_->ProcessData(shifted_data_, mr_output_))
		return false;

	if(settings_.need_precise)
	{
		if (!precise_resampler_->ProcessData(mr_output_, processed_data_))
			return false;
	}
	else
		mr_output_.swap(processed_data_);

	return true;
}

void aqua_resampler::ResamplerManager::SetSettings(const ResamplerSettings& settings)
{
	settings_ = settings;
	precise_resampler_	->SetSettings(settings_);
	mr_resampler_		->SetSettings(settings_);
}

std::vector<Ipp32fc>& ResamplerManager::GetProcessedData() {
	return processed_data_;
}

void ResamplerManager::FreeResources() {
	
	//Чистим наших работников
	mr_resampler_->Clear();	
	precise_resampler_->Clear();	
	freq_shifter_.Clear();

	//Очищаем буфферы
	processed_data_.clear();
	shifted_data_.clear();
}