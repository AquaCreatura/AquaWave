#ifndef PipeInterface_DEFINE
#define PipeInterface_DEFINE
#pragma once
#include <memory>
#include <vector>
#include <ipps.h>
namespace pipes {

struct PipeMeta
{
	int fft;


};


class PipeInterface
{
public:
	void AddNextPipe(std::shared_ptr<PipeInterface> & next_pipe) {
		next_ = next_pipe;
	}
	virtual void ProcessData(std::vector<Ipp32fc> &data_32fc) {};
	virtual void ProcessData(std::vector<Ipp32f>  &data_32f) {};
protected:
	std::shared_ptr<PipeInterface> next_;
	std::shared_ptr<PipeMeta> meta_data_;
};

class ResultPipe : public PipeInterface
{
public:
	virtual void		ProcessData(std::vector<Ipp32f>  &data_32f);
public:
	std::vector<Ipp32f> GetProcessed32f();
protected:
	std::vector<Ipp32f> processed_data_;

};

void BindPipeLine(std::vector<std::shared_ptr<PipeInterface>> pipe_line);

}
#endif // PipeInterface_DEFINE
