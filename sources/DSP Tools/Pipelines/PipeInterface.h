#ifndef PipeInterface_DEFINE
#define PipeInterface_DEFINE
#pragma once
#include <memory>
#include <vector>
#include <ipps.h>
namespace pipes {

struct PipeHolder {
	using sptr = std::shared_ptr<PipeHolder>;
	std::vector<Ipp32fc> complex_float_data;
	std::vector<Ipp32f>  float_data;

	std::vector<Ipp32fc> buffer_32fc;
};


class PipeInterface
{
public:
	using sptr = std::shared_ptr<PipeInterface>;
	void AddNextPipe(PipeInterface::sptr & next_pipe) {
		next_ = next_pipe;
	}
	virtual void ProcessData(PipeHolder::sptr meta_data) {};
protected:
	PipeInterface::sptr next_;
};

struct SimplePipeLine {
	void AddNextPipe(PipeInterface::sptr new_pipe);
	void Process(std::vector<Ipp32fc> &passed_data);
	void Process(std::vector<Ipp32f> &passed_data);

	std::vector<PipeInterface::sptr> pipes;
	PipeHolder::sptr meta;
};

}
#endif // PipeInterface_DEFINE
