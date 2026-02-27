#include "PipeInterface.h"
using namespace pipes;

void pipes::SimplePipeLine::AddNextPipe(PipeInterface::sptr new_pipe)
{
	if (!new_pipe) return;
	if(pipes.size()) pipes.back()->AddNextPipe(new_pipe);
	pipes.push_back(new_pipe);
}

void pipes::SimplePipeLine::Process(std::vector<Ipp32fc>& passed_data)
{
	if (pipes.empty()) return;
	if (!meta) meta = std::make_shared<PipeSimpleMeta>();
	meta->complex_float_data = passed_data;
	pipes.front()->ProcessData(meta);
}

void pipes::SimplePipeLine::Process(std::vector<Ipp32f>& passed_data)
{
	if (pipes.empty()) return;
	if (!meta) meta = std::make_shared<PipeSimpleMeta>();
	meta->float_data = passed_data;
	pipes.front()->ProcessData(meta);
}