#pragma once
#ifndef ARK_DEFINES
#define ARK_DEFINES




#include <memory>
#include <vector>
#include <list>
#include <qwidget.h>
#include <qdialog.h>
#include "aqua_defines.h"
#include "Tools/utility_aqua.h"
namespace fluctus
{
    class ArkInterface;

    template <typename W>
    struct WeakPtrHash {
        std::size_t operator()(const std::weak_ptr<W>& wp) const {
            auto sp = wp.lock();
            return std::hash<W*>()(sp.get());
        }
    };
    template <typename W>
    struct WeakPtrEqual {
        bool operator()(const std::weak_ptr<W>& lhs, const std::weak_ptr<W>& rhs) const {
            return !lhs.owner_before(rhs) && !rhs.owner_before(lhs);
        }
    };



    typedef  std::shared_ptr<ArkInterface> ArkSptr;
    typedef  std::weak_ptr  <ArkInterface> ArkWptr;
    typedef  std::list<ArkSptr>            StrongFleet;
    typedef  std::list<ArkWptr>            WeakFleet;
 
    //Main unit to communicate between arks
    enum ArkType
    {
        kUnknown = 0,
        kSpectrumDpx ,
        kFileSource  ,
        kFileSpectrogram
    };


    struct DoveParrent 
    {
        //thoughts can be implemented with (|) ~ (OR)
        typedef int64_t thoughts_list;

        ArkSptr sender; //who sent a dove
        enum DoveThought : int64_t
        {
            kNothing        = 0,                                //Ignore thought
            kTieBehind      = 1 << 0 , kTieFront    = 1 << 1,   //To connect arks
            kUntieFront     = 1 << 2 , kUntieBehind = 1 << 3,   //To disconnect arks
            kGetDialog      = 1 << 5 , kReset       = 1 << 6,   //Request dialog and request for reset
            kSpecialThought = 1 << 31
        };
        thoughts_list                                   base_thought  = kSpecialThought ; //Basic 
        thoughts_list                                   special_thought {0}; //Child
        ArkSptr                                         target_ark     ; //To operate with another ark
        std::shared_ptr<QWidget>                        show_widget; //Pointer to the dialog
        virtual ~DoveParrent() = default;  // virtual destructor to make polymorphic inheritance
    };

    using DoveSptr = std::shared_ptr<DoveParrent> ;

    struct DataInfo
    {
    public:
        fluctus::freq_params    freq_info_;
        std::vector<uint8_t>    data_vec;
        double                  time_point;
    };

    struct WorkBounds
    {
        Limits<double>  source;
        Limits<double>  scaled;                  
    };

}

#endif // ARK_DEFINES