#include "PipeInterface.h"
using namespace pipes;

void pipes::ResultPipe::ProcessData(std::vector<Ipp32f>& data_32f)
{
	processed_data_ = data_32f;
}

std::vector<Ipp32f> pipes::ResultPipe::GetProcessed32f()
{
	return processed_data_;
}

void pipes::BindPipeLine(std::vector<std::shared_ptr<PipeInterface>> pipe_line)
{
	if (pipe_line.empty()) return;
	for (int i = 1; i < pipe_line.size(); i++)
		pipe_line[i - 1]->AddNextPipe(pipe_line[i]);
	if (!std::dynamic_pointer_cast<ResultPipe>(pipe_line.back()))
		pipe_line.push_back(std::shared_ptr<ResultPipe>());
}
