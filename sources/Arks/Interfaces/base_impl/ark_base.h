#pragma once
#include "../ark_interface.h"

#include <tbb/spin_mutex.h>

namespace aqua
{
    //Basic implementation
    class ArkBase : public ArkInterface
    {
    public:
        ArkBase() = default;
        virtual ~ArkBase() = default;

        //Main function, which allow arks to communicate
        virtual bool           PostDove     (DoveSptr const & sent_dove) override;
        virtual StrongFleet    GetSourceArks()  override;
        virtual StrongFleet    GetSinkArks  ()  override;
        virtual StrongFleet    GetCmdArks   ()  override;
        virtual ArkType        GetArkType   () const  override;
        virtual bool SendData(aqua::DataInfo const& data_info) override { return false; };
    protected:
        StrongFleet WeakToStrongFleet(WeakFleet& weak_fleet);
        bool        AddElemToTheFleet(bool do_add, WeakFleet& fleet, const ArkSptr& target_val);
    private:
        WeakFleet sink_fleet_;
        WeakFleet source_fleet_;
        WeakFleet cmd_fleet_;
        tbb::spin_mutex con_mutex_; //Can be useful to operate with connections
    };

}
