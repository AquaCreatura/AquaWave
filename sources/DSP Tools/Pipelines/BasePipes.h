#pragma once
#include "PipeInterface.h"
#include "DSP Tools/FFT/FFT_Worker.h"
#include "DSP Tools/basic/acf_worker.h"
namespace pipes {


class FFtPipe : public PipeInterface
{
public:
	void ProcessData(PipeHolder::sptr meta_data) override;
protected:
	FFT_Worker                  fft_worker_;
};

class AcfPipe : public PipeInterface
{
public:
	void ProcessData(PipeHolder::sptr meta_data) override;
protected:
	AcfWorker	acf_worker_;
};

class SamplesDiffPipe : public PipeInterface
{
public:
	void ProcessData(PipeHolder::sptr meta_data) override;
};

class EnvelopePipe: public PipeInterface
{
public:
	void ProcessData(PipeHolder::sptr meta_data) override;
};

class PhasorPipe: public PipeInterface
{
public:
	void ProcessData(PipeHolder::sptr meta_data) override;
};

class MulByItSelfPipe : public PipeInterface
{
public:
	void ProcessData(PipeHolder::sptr meta_data) override;
};

class PowerToDbPipe : public PipeInterface
{
public:
	void ProcessData(PipeHolder::sptr meta_data) override;
};

class PrecisedPartSaver: public PipeInterface
{
public:
	PrecisedPartSaver(double start_ratio, double end_ratio);
	void ProcessData(PipeHolder::sptr meta_data) override;
protected:
	double start_ratio_;
	double end_ratio_;
};

class ZeroFirstSamples: public PipeInterface
{
public:
	ZeroFirstSamples(const double zero_ratio);
	void ProcessData(PipeHolder::sptr meta_data) override;
protected:
	const double zero_ratio_;
};

}