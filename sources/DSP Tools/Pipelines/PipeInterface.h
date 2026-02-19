#ifndef PipeInterface_DEFINE
#define PipeInterface_DEFINE



#include <memory>
#include <vector>
#include <ipps.h>

struct PipeMeta 
{
	int fft;


};


class PipeInterface 
{
public:
	void AddNextPipe(std::shared_ptr<PipeInterface> & next_pipe, std::shared_ptr<PipeMeta> & passed_meta) {
		next_ = next_pipe;
		meta_data_ = passed_meta;
	}
	virtual void ProcessData(std::vector<Ipp32fc> &data_32fc) {};
	virtual void ProcessData(std::vector<Ipp32f>  &data_32f) {};
	virtual std::vector<Ipp32f> GetProcessed32f() { return {}; };
protected:
	std::shared_ptr<PipeInterface> next_;
	std::shared_ptr<PipeMeta> meta_data_;
};

#endif // PipeInterface_DEFINE