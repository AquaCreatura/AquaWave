#pragma once

#include "../ModulationPoints.h"

#include <ipps.h>

#include <cstdint>
#include <string>
#include <vector>

namespace aq_mod
{
	// Constellation modulator: maps a bit stream to IQ samples.
	//
	// Init() builds a "bit combination -> IQ point" lookup table from the
	// constellation returned by aq_demod::CalculateModulationPivots(). Process()
	// maps bits to constellation points, upsamples by zero insertion
	// (ippsSampleUp_32fc) and shapes them with a root-raised-cosine filter
	// (ippsFIRSR_32fc). An optional lowpass can follow the RRC (EnableAfterFilter,
	// on by default). The FIR state is kept between calls, so a stream can be
	// processed block by block.
	class CcmModulator
	{
	public:
		CcmModulator();
		~CcmModulator();

		// modulation - constellation name ("BPSK", "QPSK", "QAM32", ...).
		// upsample   - samples per symbol.
		bool Init(const char* modulation, int upsample = 4);
		void Reset();

		// Enables/disables the extra lowpass after the shaping RRC (on by default).
		// May be called before or after Init().
		void EnableAfterFilter(bool enabled);
		bool IsAfterFilterEnabled() const { return after_filter_enabled_; }

		// bits   - vector of 0/1 values.
		// out_iq - shaped IQ samples, 'upsample' samples per symbol.
		bool Process(const std::vector<uint8_t>& bits, std::vector<Ipp32fc>& out_iq);

		// Converts raw bytes into a vector of bits (MSB first).
		static std::vector<uint8_t> BytesToBits(const std::vector<uint8_t>& bytes);

		bool IsInitialized() const { return !lookup_.empty(); }
		int  GetUpsample() const { return upsample_; }
		int  GetBitsPerSymbol() const { return bits_per_symbol_; }
		int  GetSymbolCount() const { return static_cast<int>(lookup_.size()); }

	private:
		// One streaming FIR filter: coefficients, delay line state and an IPP
		// work buffer. An empty filter (no taps) acts as a pass-through copy.
		struct FirStage
		{
			bool Init(const std::vector<Ipp32fc>& taps);
			void Free();
			void Reset();
			bool Process(const Ipp32fc* src, Ipp32fc* dst, int count);

			IppsFIRSpec_32fc* spec = nullptr;
			Ipp8u* buf = nullptr;
			std::vector<Ipp32fc> delay;
			int len = 0;
		};

		void BuildLookupTable(const std::vector<Ipp32fc>& pivots);
		bool BuildRrcFilter();
		bool BuildAfterFilter();

	private:
		std::vector<Ipp32fc> lookup_;    // bit group value -> constellation point
		int bits_per_symbol_ = 0;
		int upsample_ = 4;

		FirStage rrc_;                   // Shaping root-raised-cosine filter.
		FirStage after_;                 // Optional lowpass after the RRC.
		bool after_filter_enabled_ = true;

		// Scratch buffers reused between calls.
		std::vector<Ipp32fc> symbols_;
		std::vector<Ipp32fc> upsampled_;
		std::vector<Ipp32fc> shaped_;
	};
}
