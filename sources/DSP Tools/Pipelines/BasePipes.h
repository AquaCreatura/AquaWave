#include "PipeInterface.h"
#include "DSP Tools\FFT\FFT_Worker.h"
#include "DSP Tools\basic\acf_worker.h"
class FFtPipe: public PipeInterface
{
public:
	void ProcessData(std::vector<Ipp32fc> &data_32fc) override;
	std::vector<Ipp32f> GetProcessed32f() override;
protected:
	FFT_Worker                  fft_worker_;
	std::vector<Ipp32f>			processed_data_;
};


class AcfPipe : public PipeInterface
{
public:
	void ProcessData(std::vector<Ipp32fc> &data_32fc) override;
	std::vector<Ipp32f> GetProcessed32f() override;
protected:
	AcfWorker					acf_worker_;
	std::vector<Ipp32f>			processed_data_;
};

