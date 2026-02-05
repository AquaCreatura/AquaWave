#pragma once
#ifndef FILE_SRC_DEFINES
#define FILE_SRC_DEFINES
#include <ipps.h>
#include <stdint.h>
#include <qstring.h>
#include "Arks\Interfaces\ark_interface.h"
using namespace utility_aqua;
namespace file_source
{

	struct file_params
	{
		QString     file_name_;
		int64_t     carrier_hz{ 1'000'000 };
		int64_t     samplerate_hz{ 100'000 };
		IppDataType data_type_{ ipp16sc };
		int64_t     count_of_samples;
		bool        is_signal{ true };

		bool operator==(const file_params& b) const
		{
			return file_name_ == b.file_name_
				&& carrier_hz == b.carrier_hz
				&& samplerate_hz == b.samplerate_hz
				&& data_type_ == b.data_type_
				&& count_of_samples == b.count_of_samples
				&& is_signal == b.is_signal;
		}
		file_params& operator=(const file_params& b)
		{
			if (this == &b) return *this;
			file_name_ = b.file_name_;
			carrier_hz = b.carrier_hz;
			samplerate_hz = b.samplerate_hz;
			data_type_ = b.data_type_;
			count_of_samples = b.count_of_samples;
			is_signal = b.is_signal;
			return *this;
		}
	};

    struct FileSrcDove : public fluctus::DoveParrent
    {
        FileSrcDove() { base_thought = fluctus::DoveParrent::kSpecialThought; };
        enum FileSrcDoveThought : int64_t
        {
            kUnknown = 0, 
            kInitReaderInfo       = 1 << 0, //To init new reader with passed samplerate_hz and carrier_hz
            kAskChunkAround		  = 1 << 1, //To get chunk around passed point
            kAskChunksInRange     = 1 << 2, //To get chunks in range
            kAskWholeInRange	  = 1 << 3, //To get data, which is included inside passed points
            kGetFileInfo          = 1 << 4, //To get description of the current file
            kSetFileName          = 1 << 5, //To set file from command line
        };
        aqua_opt<int64_t>       data_size;
        aqua_opt<double>        time_point_start;   //Base point of the data, we are trying to read
        aqua_opt<double>        time_point_end;     //End point of the 
        aqua_opt<int64_t>       samplerate_hz;      //samplerate_hz need
        aqua_opt<int64_t>       carrier_hz;         //carrier_hz need
        aqua_opt<file_params>   file_info;        //file description
    };
    inline size_t GetSampleSize(IppDataType type)
    {
        switch (type) 
        {
            case ipp1u:    return 1;
            case ipp8u:    return 1;
            case ipp8uc:   return 2; // комплексный: 2 * 1 байт
            case ipp8s:    return 1;
            case ipp8sc:   return 2; // комплексный: 2 * 1 байт
            case ipp16u:   return 2;
            case ipp16uc:  return 4; // комплексный: 2 * 2 байта
            case ipp16s:   return 2;
            case ipp16sc:  return 4; // комплексный: 2 * 2 байта
            case ipp32u:   return 4;
            case ipp32uc:  return 8; // комплексный: 2 * 4 байта
            case ipp32s:   return 4;
            case ipp32sc:  return 8; // комплексный: 2 * 4 байта
            case ipp32f:   return 4;
            case ipp32fc:  return 8; // комплексный: 2 * 4 байта
            case ipp64u:   return 8;
            case ipp64uc:  return 16; // комплексный: 2 * 8 байт
            case ipp64s:   return 8;
            case ipp64sc:  return 16; // комплексный: 2 * 8 байт
            case ipp64f:   return 8;
            case ipp64fc:  return 16; // комплексный: 2 * 8 байт
            default:       return 0; // неопределенный или неподдерживаемый тип
        };
    };
}

#endif // FILE_SRC_DEFINES