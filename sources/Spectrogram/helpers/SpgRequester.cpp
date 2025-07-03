#include "File Source/file_souce_defs.h"
#include "GUI/basic tools/gui_helper.h"
#include "SpgRequester.h"
#include <qmessagebox.h>
#include <ippvm.h>
#include <thread>
using namespace spg_core; // Используем пространство имён dpx_core

spg_core::SpgRequester::SpgRequester(const spg_data & spg, const WorkBounds& time_bounds) : 
    spg_(spg), time_bounds_(time_bounds)
{
}

void spg_core::SpgRequester::Initialise(const ArkWptr & file_source, const ArkWptr & ark_spg)
{
    ark_file_src_   = file_source;
    ark_spg_        = ark_spg;
}


bool spg_core::SpgRequester::SendRequestDove(const request_params & req_info)
{
    if(!req_info.need_request) return true;
    const auto file_src = ark_file_src_.lock();
    const auto base_ark = ark_spg_.lock();
    if(!file_src || !base_ark) return false;

    auto req_dove = std::make_shared<file_source::FileSrcDove>();
    req_dove->base_thought      = fluctus::DoveParrent::DoveThought::kSpecialThought;
    req_dove->special_thought   = file_source::FileSrcDove::kInitReaderInfo |  file_source::FileSrcDove::kAskSingleDataAround;
    req_dove->target_ark        = base_ark;
    req_dove->time_point_start  = req_dove->time_point_end = req_info.time_point;
    req_dove->data_size         = req_info.data_size;
    if (!file_src->SendDove(req_dove))
    {
        QMessageBox::warning( nullptr, "Cannot Request Data", "Do something with file source");
        return false;
    }
    return true;
}

SpgRequester::request_params SpgRequester::GetRequestParams() {
    request_params req_info; // Initialize request parameters structure
    auto &data = spg_.base_data; // Reference to base data for convenience
    const auto &hor_bounds = data.val_bounds.horizontal; // Horizontal value bounds
    const auto &src_bounds = hor_bounds; // Source time bounds

    // Calculate normalized request bounds relative to source time bounds
    const Limits<double> req_bounds = {
        (hor_bounds.low - src_bounds.low) / src_bounds.delta(),
        (hor_bounds.high - src_bounds.low) / src_bounds.delta()
    };

    req_info.data_size = data.size.vertical; // Set vertical data size (1:1 mapping)
    const int hor_size = data.size.horizontal; // Horizontal data size
    const auto &relevant_vec = data.relevant_vec; // Vector indicating relevant data points

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
        req_info.need_request = false;
        return req_info;
    }

    // Calculate time point for request based on pixel position
    const double draw_pos_ratio = static_cast<double>(res_draw_location) / hor_size;
    req_info.time_point = req_bounds.low + draw_pos_ratio * req_bounds.delta();

    return req_info; // Return completed request parameters
}
void spg_core::SpgRequester::RequestData()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const auto request_info = GetRequestParams();
    SendRequestDove(request_info);
}

