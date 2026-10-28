#pragma once
#include "PipeInterface.h"
#include "DSP Tools/FFT/FFT_Worker.h"
#include "DSP Tools/basic/acf_worker.h"
#include "DSP Tools/Resampler/ResamplerMan.h"
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
	void ProcessData(Ipp32f* data, size_t data_size);
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

class MulByConstPipe : public PipeInterface
{
public:
	MulByConstPipe(const float div_koeff) : mul_koeff_(div_koeff) {};
	void ProcessData(PipeHolder::sptr meta_data) override;
protected:
	const float mul_koeff_;
};

class LinearScalePipe: public PipeInterface
{
public:
	LinearScalePipe(const float target_scale = 1.f) : target_koeff_(target_scale) {};
	void ProcessData(PipeHolder::sptr meta_data) override;
protected:
	const float target_koeff_;
};



class SqrtPipe: public PipeInterface
{
public:
	void ProcessData(PipeHolder::sptr meta_data) override;
};
class ResamplerPipe : public PipeInterface
{
public:
	ResamplerPipe(int64_t passed_sr, int64_t need_sr, int64_t need_bw);
	ResamplerPipe(int64_t passed_fc, int64_t passed_sr, int64_t need_fc, int64_t need_sr, int64_t need_bw);
	void ProcessData(PipeHolder::sptr meta_data) override;
protected:
	aqua_resampler::ResamplerManager man_;
};
class DcRemovePipe : public PipeInterface
{
public:
	void ProcessData(PipeHolder::sptr meta_data) override;
};
class RmsNormPipe: public PipeInterface
{
public:
	RmsNormPipe(float target_val = 1.0f):upscale_val_(target_val) {};
	void ProcessData(PipeHolder::sptr meta_data) override;
protected:
	float upscale_val_ = 1.f;

};
}