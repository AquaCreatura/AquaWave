#include "HarmonicEstimator.h"

HarmonicEstimator::HarmonicEstimator()
{
	Reset();
}

void HarmonicEstimator::SetBandwidth(const double bw_hz)
{
	bw_hz_ = bw_hz;
}

void HarmonicEstimator::SetBaseCarrier(const double fc_hz)
{
	data_carrier_hz_ = fc_hz;
	carrier_hz_ = data_carrier_hz_;
}

void HarmonicEstimator::SetLocalisedPeaks(analz_chart_type type, std::vector<double>& peaks)
{
	storage_[type] = peaks;
}

void HarmonicEstimator::Estimate()
{
	aqua_opt<double> opt_matched_bd_hz, opt_psk_bd_hz, opt_am_bd_hz;
	aqua_opt<double> opt_carrier_hz;
	EstimateSymbolRate(opt_matched_bd_hz, opt_psk_bd_hz, opt_am_bd_hz);

	if (opt_matched_bd_hz) {
		symbol_rate_hz_ = opt_matched_bd_hz.value();
	}
	else if ((opt_psk_bd_hz || opt_am_bd_hz)) {
		symbol_rate_hz_ = std::max(opt_psk_bd_hz.value(), opt_am_bd_hz.value());
	}
	EstimateCarrier(opt_carrier_hz);
	if (opt_carrier_hz) {
		carrier_hz_ = opt_carrier_hz.value();
	}
	EstimateAcf(acf_msec_);
}

void HarmonicEstimator::Reset()
{
	storage_.clear();
	symbol_rate_hz_.reset();
	carrier_hz_.reset();
	acf_msec_.reset();


	bw_hz_ = 0.;
	data_carrier_hz_ = 0.;
	strength_symbol_rate_hz_ = 0;
	strength_carrier_hz_ = 0;

}

void HarmonicEstimator::EstimateSymbolRate(aqua_opt<double>& matched_bd_hz, aqua_opt<double>& best_psk_hz, aqua_opt<double>& best_env_hz)
{
	auto &psk_vec = storage_[analz_chart_type::kPhasorSpectrum]; 
	auto &env_vec = storage_[analz_chart_type::kEnvelopeSpectrum];
	
	//Находим совпадение
	if(!psk_vec.empty() && !env_vec.empty())
	{
		std::vector<float> matched_vec;
		for (auto it_psk : psk_vec) {
			for (auto it_env : env_vec) {
				if (std::abs(it_psk - it_env) < 0.001 * bw_hz_) {
					matched_vec.push_back(it_psk);
				}
			}
		} //Хорошо бы ещё учитывать силу пика 
		if (!matched_vec.empty()) {
			matched_bd_hz = matched_vec.back();
			if (symbol_rate_hz_ && std::abs(symbol_rate_hz_.value() - matched_bd_hz) < 0.001 * bw_hz_) {
				strength_symbol_rate_hz_ = 1;
			}
			
			return;
		}
	}
	
	//Ещё можно исходить из символьной от спектра степени
	
	//Крайние варианты
	if (strength_symbol_rate_hz_ < 1) {
		if (!psk_vec.empty()) {
			best_psk_hz = psk_vec.back();
		}
		if (!env_vec.empty()) {
			best_env_hz = env_vec.back();
		}
	}
}

void HarmonicEstimator::EstimateCarrier(aqua_opt<double>& mid_fc_hz)
{
	bool is_centered = false;
	auto& cur_storage = storage_[analz_chart_type::kPowerSpectrum];
	//Самый лучший вариант
	if(cur_storage.size() >= 3)
	{
		for (int left_idx = 0; left_idx < cur_storage.size() - 2; left_idx++) {
			int mid_idx = left_idx + 1; int right_idx = mid_idx + 1;
			auto left_side = cur_storage[mid_idx] - cur_storage[left_idx];
			auto right_side = cur_storage[right_idx] - cur_storage[mid_idx];
			if (std::abs(left_side - right_side) < 0.01 * bw_hz_) {
				is_centered = true;
				mid_fc_hz = cur_storage[mid_idx];
				strength_carrier_hz_ = 1;
				return;
			}
		}
	}
	//Есть ещё вариант, если заключена символьная...

	//Находим ближайшую к центру
	if (strength_carrier_hz_ < 1) {
		double closest_hz = 0;
		{
			auto min_delta = DBL_MAX;
			for (auto& it : cur_storage) {
				auto cur_delta = std::abs(it - data_carrier_hz_);
				if (cur_delta < min_delta) {
					min_delta = cur_delta;
					closest_hz = it;
				}
			}
		}
		mid_fc_hz = closest_hz;
	}
}

void HarmonicEstimator::EstimateAcf(aqua_opt<double>& acf_msec)
{
	auto& cur_storage = storage_[analz_chart_type::kAcf];
	if (cur_storage.empty())
		return;

	if (cur_storage.size() == 1) {
		acf_msec = cur_storage.front();
	}

	double best_fund = 0;
	size_t best_support = 0;

	int max_harmonic = 8; 
	double tolerance_ratio = 0.01;

	for (float candidate : cur_storage)
	{
		size_t support = 1;
		for (float other : cur_storage)
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

	if (best_support >= 1) {
		acf_msec = best_fund;
	}
	else {
		acf_msec = cur_storage.front();
	}
	return;	
}

bool HarmonicEstimator::IsHarmonicOf(float value, float pivot, double tolerance, int max_harmonic)
{
	if (pivot == 0 || value == 0)
		return false;

	double ratio = static_cast<double>(value) / static_cast<double>(pivot);
	int nearest = static_cast<int>(std::round(ratio));
	if (nearest < 1 || nearest > max_harmonic)
		return false;

	double expected = nearest * pivot;
	double error = std::abs(static_cast<double>(value) - expected) / static_cast<double>(value);
	return error <= tolerance;
}
