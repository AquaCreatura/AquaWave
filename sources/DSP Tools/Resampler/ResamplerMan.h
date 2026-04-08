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

		void SetSettings(const ResamplerSettings& settings);
		void FreeResources();

		bool SetBaseParams(const int64_t carrier_hz, const int64_t samplerate_hz);
		bool SetTargetParams(const int64_t target_fc_hz, int64_t& target_fs_hz, const int64_t target_bw_hz);


		bool ProcessBlock(const Ipp32fc* input_data, size_t size);
		std::vector<Ipp32fc>& GetProcessedData();

	private:
		//Пераметры настройки
		ResamplerSettings                   settings_;
		double                              resample_ratio_;
		fluctus::freq_params				base_params_;

		//Наши работяги
		std::unique_ptr<ResamplerInterface> mr_resampler_;
		std::unique_ptr<ResamplerInterface> precise_resampler_;
		aqua_dsp_tools::FrequencyShifter    freq_shifter_;

		//Наши буфферы
		std::vector<Ipp32fc>                processed_data_;
		std::vector<Ipp32fc>                shifted_data_;
		std::vector<Ipp32fc>				mr_output_;
	};

}