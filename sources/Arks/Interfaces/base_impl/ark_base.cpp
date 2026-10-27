#include "ark_base.h"
using namespace aqua;
bool aqua::ArkBase::PostDove(DoveSptr const & sent_dove)
{
    if (!sent_dove)
        throw std::invalid_argument("Not created message sent!");
    auto target_val = sent_dove->target_ark;

    auto base_thought = sent_dove->base_thought;
    if      (base_thought == DoveParrent::DoveThought::kNothing)
        return true;
    //connect input connection
    else if (base_thought & DoveParrent::DoveThought::kAddSource)
    {
        AddElemToTheFleet(true, source_fleet_, target_val);
    }
    else if (base_thought & DoveParrent::DoveThought::kAddSink)
    {   
        AddElemToTheFleet(true, sink_fleet_, target_val);
    }
    else if (base_thought & DoveParrent::DoveThought::kAddCmd)
    {
        AddElemToTheFleet(true, cmd_fleet_, target_val);
    }
    else if (base_thought & DoveParrent::DoveThought::kRemoveSource)
    {
        AddElemToTheFleet(false, source_fleet_, target_val);
    }
    else if (base_thought & DoveParrent::DoveThought::kRemoveSink)
    {
        AddElemToTheFleet(false, sink_fleet_, target_val);
    }
    else if (base_thought & DoveParrent::DoveThought::kRemoveCmd)
    {
        AddElemToTheFleet(false, cmd_fleet_, target_val);
    }

    return true;
}

aqua::StrongFleet aqua::ArkBase::GetSourceArks()
{
    return WeakToStrongFleet(source_fleet_);
}

aqua::StrongFleet aqua::ArkBase::GetSinkArks()
{
    return WeakToStrongFleet(sink_fleet_);
}

StrongFleet aqua::ArkBase::GetCmdArks()
{
    return WeakToStrongFleet(cmd_fleet_);
}

ArkType aqua::ArkBase::GetArkType() const
{
    return ArkType::kUnknown;
}

StrongFleet aqua::ArkBase::WeakToStrongFleet(WeakFleet& weak_fleet)
{
    tbb::spin_mutex::scoped_lock guard_con(con_mutex_);
    //Can not be this situation - exception!
    if (false)
    {
        //Remove expired
        weak_fleet.remove_if([](const aqua::ArkWptr& passed_param)
            {
                return passed_param.expired();
            });
    }

    StrongFleet strong_fleet;
    for (const auto& weak_ptr : weak_fleet)
    {
        if (auto locked_ptr = weak_ptr.lock())
        {
            strong_fleet.push_back(std::move(locked_ptr));
        }
    }
    return strong_fleet;
  
}

bool aqua::ArkBase::AddElemToTheFleet(bool do_add, WeakFleet& fleet, const ArkSptr& target_val)
{
    if (!target_val)
        throw std::invalid_argument("Wrong target ark!");

    tbb::spin_mutex::scoped_lock guard_con(con_mutex_);

    if (do_add)
    {
        // Проверяем, нет ли уже такого элемента
        auto found_elem = std::find_if(fleet.begin(), fleet.end(),
            [&](const aqua::ArkWptr& elem) {
                auto locked = elem.lock();
                return locked && locked == target_val;
            });

        if (found_elem != fleet.end())
            throw std::logic_error("Element already exists!");

        fleet.push_back(target_val);
    }
    else
    {
        // Удаляем элемент
        auto found_elem = std::find_if(fleet.begin(), fleet.end(),
            [&](const aqua::ArkWptr& elem) {
                auto locked = elem.lock();
                return locked && locked == target_val;
            });

        if (found_elem == fleet.end())
            throw std::logic_error("Can not delete non-existent ark!");

        fleet.erase(found_elem);
    }

    return true;
}
