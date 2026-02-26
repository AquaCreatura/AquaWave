#pragma once
#include "PipeInterface.h"
#include "DSP Tools\FFT\FFT_Worker.h"
#include "DSP Tools\basic\acf_worker.h"
namespace pipes {


class FFtPipe : public PipeInterface
{
public:
	void ProcessData(std::vector<Ipp32fc> &data_32fc) override;
protected:
	FFT_Worker                  fft_worker_;
};

class AcfPipe : public PipeInterface
{
public:
	void ProcessData(std::vector<Ipp32fc> &data_32fc) override;
protected:
	AcfWorker	acf_worker_;
};

class SamplesDiffPipe : public PipeInterface
{
public:
	void ProcessData(std::vector<Ipp32fc> &data_32fc) override;
};

class EnvelopePipe: public PipeInterface
{
public:
	void ProcessData(std::vector<Ipp32fc> &data_32fc) override;
};

class PhasorPipe: public PipeInterface
{
public:
	void ProcessData(std::vector<Ipp32fc> &data_32fc) override;
};

class MulByItSelfPipe : public PipeInterface
{
public:
	void ProcessData(std::vector<Ipp32fc> &data_32fc) override;
};

}