#pragma once
#include <mutex>
#include <QWidget>

#include "Interfaces/ark_interface.h"

namespace fluctus
{
    //Basic implementation
    class ArkBase : public ArkInterface
    {
    public:
        ArkBase() = default;
        virtual ~ArkBase() = default;

        //Main function, which allow arks to communicate
        virtual bool           SendDove     (DoveSptr const & sent_dove) override;
        virtual StrongFleet    GetBehindArks()  override;
        virtual StrongFleet    GetFrontArks ()  override;
        virtual bool SendData(fluctus::DataInfo const& data_info) override { return false; };
    private:
        WeakFleet front_fleet_;
        WeakFleet behind_fleet_;
        std::mutex con_mutex_; //Can be useful to operate with connections
    };

}