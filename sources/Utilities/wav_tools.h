#pragma once

#include <fstream>
#include <cstdint>
#include <string>

#include "utility_aqua.h"

using namespace utility_aqua;
namespace utility_aqua {
	struct WavInfo
	{
		aqua_opt<uint16_t> audio_format;
		aqua_opt<uint16_t> num_channels;
		aqua_opt<uint32_t> sample_rate;
		aqua_opt<uint32_t> byte_rate;
		aqua_opt<uint16_t> block_align;
		aqua_opt<uint16_t> bits_per_sample;

		aqua_opt<uint32_t> data_offset;
		aqua_opt<uint32_t> data_size;
	};



	bool get_wav_info(const std::string& filename, WavInfo& out);
	std::string MakeWavPrefix(const WavInfo& info);
	WavInfo DefaultWav(const int64_t SampleRate);
	bool UpdateWavDataSize(const std::string& file_path);

}