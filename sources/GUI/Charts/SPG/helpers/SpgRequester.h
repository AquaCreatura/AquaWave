#pragma once
#include "Arks\Interfaces\ark_interface.h"
#include "GUI/Charts/SPG/SPG core/spg_defs.h"
#include <future>
#include <qtimer.h>

using namespace fluctus;
namespace spg_core
{
//Class, which request data from the file source
class SpgRequester : public QObject
{
    Q_OBJECT
public:
    SpgRequester(const spg_data& spg, const WorkBounds& time_bounds);
	~SpgRequester();
    void Initialise	 (const ArkWptr& file_source, const ArkWptr& ark_spg);
protected:
	void StartProcess(bool do_start);
	void LoopProcess();
    struct request_params
    {
        double  time_point      = 0.5;
        int64_t data_size       = 1024;
        bool    need_request    = false; 
    };
    bool SendRequestDove(const request_params& req_info);
    request_params GetRequestParams();
protected:
    ArkWptr                     ark_spg_;
    ArkWptr                     ark_file_src_;
    const spg_data&             spg_;
    const WorkBounds&           src_time_bounds_;
    std::vector<int>            base_draw_locations_;
    std::vector<int>            spec_draw_locations_;

	std::atomic_bool	is_running_process_	{false};
	std::future<void>	process_anchor_;

	int 				last_sent_positions_[15];
	int					sent_positions_counter_{ 0 };
};


}