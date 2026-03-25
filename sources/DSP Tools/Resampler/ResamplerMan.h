#pragma once
#include <memory>
#include "ResamplersImpl/ResamperInterface.h"
#include "../basic/freq_shifter.h"
#include "ark_defs.h"

namespace aqua_resampler
{

	class ResamplerManager
	{
	public:
		ResamplerManager();
		~ResamplerManager();

		// Единственный метод инициализации
		bool Init(const fluctus::freq_params& base_params,
			fluctus::freq_params& target_params,
			bool precise);

		bool ProcessBlock(const Ipp32fc* input_data, size_t size);
		std::vector<Ipp32fc>& GetProcessedData();
		void FreeResources();

	private:
		// Вспомогательные методы инициализации ресемплеров
		bool initMRResampler(int64_t base_rate, int64_t approx_target_rate, double mr_ratio);
		bool initPreciseResampler(int64_t approx_target_rate, int64_t target_rate);

		std::vector<Ipp32fc>                processed_data_;
		std::unique_ptr<ResamplerInterface> mr_resampler_;
		std::unique_ptr<ResamplerInterface> precise_resampler_;
		double                              resample_ratio_;

		// Раздельные настройки для MR и Precise (внутренние, не настраиваются извне)
		ResamplerSettings                   mr_settings_;
		ResamplerSettings                   precise_settings_;

		aqua_dsp_tools::FrequencyShifter    freq_shifter_;
		std::vector<Ipp32fc>                shifted_data_;
	};

}