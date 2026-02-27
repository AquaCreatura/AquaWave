#ifndef PipeInterface_DEFINE
#define PipeInterface_DEFINE
#pragma once
#include <memory>
#include <vector>
#include <ipps.h>
namespace pipes {

struct PipeSimpleMeta {
	using sptr = std::shared_ptr<PipeSimpleMeta>;


	PipeSimpleMeta(std::vector<Ipp32fc> &passed_32fc) {
		float_data = passed_32fc;
	};
	PipeSimpleMeta(std::vector<Ipp32f> &passed_32f) {
		complex_float_data = passed_32f;
	};
	std::vector<Ipp32fc> float_data;
	std::vector<Ipp32f>  complex_float_data;
};


class PipeInterface
{
public:
	void AddNextPipe(std::shared_ptr<PipeInterface> & next_pipe) {
		next_ = next_pipe;
	}
	virtual void ProcessData(PipeSimpleMeta::sptr meta_data) {};
protected:
	std::shared_ptr<PipeInterface> next_;
};

void BindPipeLine(std::vector<std::shared_ptr<PipeInterface>> pipe_line);

}
#endif // PipeInterface_DEFINE
