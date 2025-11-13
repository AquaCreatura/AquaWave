#include "File Source/file_souce_defs.h"
#include "GUI/basic tools/gui_helper.h"
#include "SpgRequester.h"
#include <qmessagebox.h>
#include <ippvm.h>
#include <thread>
using namespace spg_core; // Используем пространство имён dpx_core

spg_core::SpgRequester::SpgRequester(const spg_data & spg, const WorkBounds& time_bounds) : 
    spg_(spg), src_time_bounds_(time_bounds)
{
}

spg_core::SpgRequester::~SpgRequester()
{
	StartProcess(false);
}



void spg_core::SpgRequester::Initialise(const ArkWptr & file_source, const ArkWptr & ark_spg)
{
	StartProcess(false);
    ark_file_src_   = file_source;
    ark_spg_        = ark_spg;
	StartProcess(true);
}

void spg_core::SpgRequester::StartProcess(bool do_start)
{
	if (do_start) {
		if (is_running_process_) return; // уже работает
		is_running_process_ = true;
		process_anchor_ = std::async(std::launch::async, [this] {
			this->LoopProcess();
		});
	}
	else {
		if (!is_running_process_) return; // уже остановлено
		is_running_process_ = false;
		if (process_anchor_.valid()) {
			process_anchor_.wait(); // дождаться завершения
		}
	}
}

void spg_core::SpgRequester::LoopProcess()
{
	while (is_running_process_) {

		const auto request_info = GetRequestParams();
		if (!SendRequestDove(request_info)) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		//
	}
}




bool spg_core::SpgRequester::SendRequestDove(const request_params & req_info)
{
    if(!req_info.need_request) return false;
    const auto file_src = ark_file_src_.lock();
    const auto base_ark = ark_spg_.lock();
    if(!file_src || !base_ark) return false;

    auto req_dove = std::make_shared<file_source::FileSrcDove>();
    req_dove->base_thought      = fluctus::DoveParrent::DoveThought::kSpecialThought;
    req_dove->special_thought   = file_source::FileSrcDove::kInitReaderInfo |  file_source::FileSrcDove::kAskChunkAround;
    req_dove->target_ark        = base_ark;
    req_dove->time_point_start  = req_dove->time_point_end = req_info.time_point;
    req_dove->data_size         = req_info.data_size;

    if (!file_src->SendDove(req_dove))
    {
        return false;
    }
    return true;
}

SpgRequester::request_params SpgRequester::GetRequestParams() {
    request_params req_info; // Initialize request parameters structure
	
	if (spg_.realtime_data.state & kRequestStation) { //Сообщаем, что начали подготовку
		tbb::spin_mutex::scoped_lock guard_lock(spg_.rw_mutex_);
		if (spg_.realtime_data.state & kRequestStation)
		{
			spg_.realtime_data.state = (spg_.realtime_data.state ^ kRequestStation) | kPreparingStation;
		}
	};
	if (spg_.base_data.state & kRequestStation) { //Сообщаем, что начали подготовку
		tbb::spin_mutex::scoped_lock guard_lock(spg_.rw_mutex_);
		if (spg_.base_data.state & kRequestStation)
		{
			spg_.base_data.state = (spg_.base_data.state ^ kRequestStation) | kPreparingStation;
		}
	};




	if (spg_.base_data.state == spg_core::kFullData && spg_.realtime_data.state == spg_core::kFullData)
		return req_info;


	bool need_request_base = (spg_.base_data.state & kPreparingStation) || (spg_.realtime_data.state == spg_core::kFullData);
	auto &holder_to_request = need_request_base ? spg_.base_data : spg_.realtime_data;

	
    const auto &hor_bounds = holder_to_request.val_bounds.horizontal; // Horizontal value bounds
    const auto &src_bounds = src_time_bounds_.source; // Source time bounds

    // Calculate normalized request bounds relative to source time bounds
    const Limits<double> req_bounds = {
        (hor_bounds.low		- src_bounds.low) / src_bounds.delta(),
        (hor_bounds.high	- src_bounds.low) / src_bounds.delta()
    };
	if (need_request_base)
	{
		req_info.data_size = holder_to_request.size.vertical; // Set vertical data size (1:1 mapping)
	}
	else 
	{

		req_info.data_size = 8'192 / 2;
	}
    const int hor_size = holder_to_request.size.horizontal; // Horizontal data size
    const auto &relevant_vec = holder_to_request.relevant_vec; // Vector indicating relevant data points

    // Generate pixel locations if not already computed for horizontal size
    if (base_draw_locations_.size() != hor_size) {
        base_draw_locations_ = GetAssimLocationsVec(hor_size);
    }

    int res_draw_location = -1; // Initialize result pixel location as not found

    // Find first non-relevant pixel location
    for (const auto location : base_draw_locations_) {
        if (!relevant_vec[location]) {
            res_draw_location = location;
            break; // Exit loop once found
        }
    }

    // If all data is relevant, no request needed
    if (res_draw_location < 0) {
		tbb::spin_mutex::scoped_lock guard_lock(spg_.rw_mutex_);
		if (!(holder_to_request.state & kRequestStation))
		{
			holder_to_request.state = kFullData;
		}
        return req_info;
    }

	if (holder_to_request.state & kPreparingStation && need_request_base)
	{
		int relevant_counter = 0;
		int need_minimum = 100; 
		//В случае с real time данными, порог динамический
		if (!need_request_base) {
			double scale_koeff = spg_.base_data.val_bounds.horizontal.delta() / 
									spg_.realtime_data.val_bounds.horizontal.delta();
			need_minimum = spg_.base_data.size.horizontal / scale_koeff;
		}
		for (auto rel_iter : relevant_vec) {
			relevant_counter += rel_iter;
			if (relevant_counter > need_minimum) {
				tbb::spin_mutex::scoped_lock guard_lock(spg_.rw_mutex_);
				holder_to_request.state = spg_core::kReadyToUse;
				break;
			}
		}
		

	}
    // Calculate time point for request based on pixel position
    const double draw_pos_ratio = static_cast<double>(res_draw_location) / hor_size;
	req_info.time_point = (req_bounds.low + draw_pos_ratio * req_bounds.delta()); // src_bounds.delta();
	req_info.need_request = true;
    return req_info; // Return completed request parameters
}

