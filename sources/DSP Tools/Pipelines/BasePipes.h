#pragma once
#include "PipeInterface.h"
#include "DSP Tools\FFT\FFT_Worker.h"
#include "DSP Tools\basic\acf_worker.h"
namespace pipes {


class FFtPipe : public PipeInterface
{
public:
	void ProcessData(PipeSimpleMeta::sptr meta_data) override;
protected:
	FFT_Worker                  fft_worker_;
};

class AcfPipe : public PipeInterface
{
public:
	void ProcessData(PipeSimpleMeta::sptr meta_data) override;
protected:
	AcfWorker	acf_worker_;
};

class SamplesDiffPipe : public PipeInterface
{
public:
	void ProcessData(PipeSimpleMeta::sptr meta_data) override;
};

class EnvelopePipe: public PipeInterface
{
public:
	void ProcessData(PipeSimpleMeta::sptr meta_data) override;
};

class PhasorPipe: public PipeInterface
{
public:
	void ProcessData(PipeSimpleMeta::sptr meta_data) override;
};

class MulByItSelfPipe : public PipeInterface
{
public:
	void ProcessData(PipeSimpleMeta::sptr meta_data) override;
};

}