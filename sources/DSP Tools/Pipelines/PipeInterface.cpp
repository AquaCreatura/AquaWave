#include "PipeInterface.h"
using namespace pipes;


void pipes::BindPipeLine(std::vector<std::shared_ptr<PipeInterface>> pipe_line)
{
	if (pipe_line.empty()) return;
	for (int i = 1; i < pipe_line.size(); i++)
		pipe_line[i - 1]->AddNextPipe(pipe_line[i]);
}
