#include "ark_base.h"

bool fluctus::ArkBase::SendDove(DoveSptr const & sent_dove)
{
    if (!sent_dove)
        throw std::invalid_argument("Not created message sent!");
    auto target_val = sent_dove->target_ark;

    auto base_thought = sent_dove->base_thought;
    if      (base_thought == DoveParrent::DoveThought::kNothing)
        return true;
    //connect input connection
    else if (base_thought & DoveParrent::DoveThought::kTieBehind)
    {
        if (!target_val)
            throw std::invalid_argument("Wrong target ark! (Behind tie)");
        std::lock_guard<std::mutex> guard_con(con_mutex_);
        auto found_elem = std::find_if(behind_fleet_.begin(), behind_fleet_.end(),
            [&](fluctus::ArkWptr elem) {return elem.lock() == target_val;  });
        if (found_elem != behind_fleet_.end())
            throw std::logic_error("Element allready exists!");
        behind_fleet_.push_back(target_val);
        return true;

    }
    //connect output connection
    else if (base_thought & DoveParrent::DoveThought::kTieFront)
    {   
        if (!target_val)
            throw std::invalid_argument("Wrong target ark (front tie)");
        std::lock_guard<std::mutex> guard_con(con_mutex_);
        auto found_elem = std::find_if(front_fleet_.begin(), front_fleet_.end(), 
            [&](fluctus::ArkWptr elem) {return elem.lock() == target_val;  });
        if (found_elem != front_fleet_.end())
            throw std::logic_error("Element allready exists!");
        front_fleet_.push_back(target_val);
        return true;
    }
    //disconnect input
    else if (base_thought & DoveParrent::DoveThought::kUntieFront)
    {
        bool is_deleted = false;
        if (!target_val)
            throw std::invalid_argument("Wrong target ark (front untie)!");
        std::lock_guard<std::mutex> guard_con(con_mutex_);
        //Go through elements and delete passed target
        for (auto in_iter = front_fleet_.begin(); in_iter != front_fleet_.end(); in_iter++)
        {
            if (in_iter->lock() == target_val)
            {
                front_fleet_.erase(in_iter);
                is_deleted = true;
                break;
            }
        }
        if (!is_deleted)
            throw std::logic_error("Can not delete not existed ark!");
        return true;
    }
    //disconnect output
    else if (base_thought & DoveParrent::DoveThought::kUntieBehind)
    {
        bool is_deleted = false;
        if (!target_val)
            throw std::invalid_argument("Wrong target ark! (untie behind)");
        //Go through elements and delete passed target
        std::lock_guard<std::mutex> guard_con(con_mutex_);
        for (auto out_iter = behind_fleet_.begin(); out_iter != behind_fleet_.end(); out_iter++)
        {
            if (out_iter->lock() == (target_val))
            {
                behind_fleet_.erase(out_iter);
                is_deleted = true;
                break;
            }
        }
        if (!is_deleted)
            throw std::logic_error("Can not delete not existed ark!");
        return true;

    }
    return true;
}

fluctus::StrongFleet fluctus::ArkBase::GetBehindArks()
{
    std::lock_guard<std::mutex> guard_con(con_mutex_);
    StrongFleet res_fleet;
    //Can not be this situation - exception!
    if (false)
    {
        //Remove expired
        behind_fleet_.remove_if([](const fluctus::ArkWptr& passed_param)
            {
                return passed_param.expired();
            });
    }
    //Go through data and update info
    for (auto ark_iter = behind_fleet_.begin(); ark_iter != behind_fleet_.end(); ark_iter++)
    {
        if (auto locked_ptr = ark_iter->lock())
        {
            res_fleet.push_back(locked_ptr);
        }
    }
    return res_fleet;
}

fluctus::StrongFleet fluctus::ArkBase::GetFrontArks()
{
    std::lock_guard<std::mutex> guard_con(con_mutex_);
    StrongFleet res_fleet;
    //Can not be this situation - exception!
    if (false)
    {
        //Remove expired
        front_fleet_.remove_if([](const fluctus::ArkWptr& passed_param)
            {
                return passed_param.expired();
            });
    }
    //Go through data and update info
    for (auto ark_iter = front_fleet_.begin(); ark_iter != front_fleet_.end(); ark_iter++)
    {
        if(auto locked_ptr = ark_iter->lock())
        {
            res_fleet.push_back(locked_ptr);
        }
    }
    return res_fleet;
}
