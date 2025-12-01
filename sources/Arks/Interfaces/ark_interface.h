#pragma once

#include <qobject.h>
#include <QWidget>
#include "ark_defs.h"

namespace fluctus
{

class ArkInterface: public QObject, public std::enable_shared_from_this<ArkInterface>
{
    Q_OBJECT
public:
    typedef  std::shared_ptr<ArkInterface> sptr;
        
                        ArkInterface    () = default;
    virtual             ~ArkInterface   () = default;
    virtual ArkType     GetArkType      () const = 0;
    virtual bool        SendDove        (std::shared_ptr<DoveParrent> const & sent_dove) = 0; //Main function, which allow arks to communicate
    virtual StrongFleet GetBehindArks   () = 0;
    virtual StrongFleet GetFrontArks    () = 0;
    virtual bool        SendData        (DataInfo const & data_info) = 0;
};


}