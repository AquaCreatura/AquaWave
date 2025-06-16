#include "TimeGraphArk.h"
#include <qglobal.h>
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <filesystem>
using namespace time_graph;
// ========================================== TimeGraphArk =================================
TimeGraphArk::TimeGraphArk()
{
    //dialog_ = std::make_shared<FileSourceDialog>();
    
}

TimeGraphArk::~TimeGraphArk()
{
}

bool TimeGraphArk::SendDove(fluctus::DoveSptr const & sent_dove)
{
    if (!sent_dove)
        throw std::invalid_argument("empty dove sent!");
    const auto parrent_type = sent_dove->base_thought;
    if (parrent_type & fluctus::DoveParrent::kTieFront)
    {
        return ArkBase::SendDove(sent_dove);
    }
    if (parrent_type & fluctus::DoveParrent::kUntieFront)
    {

        return ArkBase::SendDove(sent_dove);
    }
    if (parrent_type & fluctus::DoveParrent::kGetDialog)
    {
        sent_dove->show_widget = dialog_;
    }
    if (parrent_type & fluctus::DoveParrent::kSpecialThought)
    {
        //auto file_src_dove = std::make_shared<FileSrcDove>();//std::dynamic_pointer_cast<FileSrcDove>(sent_dove);
        //if (!file_src_dove)
        //    throw std::invalid_argument("wrong thought type!");
        //const auto file_src_thoght = file_src_dove->special_thought;
        //if (file_src_thoght & FileSrcDove::FileSrcDoveThought::kAskSingleData)
        //{
        //    //Do smth
        //}
        //if (file_src_thoght & FileSrcDove::FileSrcDoveThought::kAskCyclicData)
        //{
        //    //Do smth

        //}

    }
    return ArkBase::SendDove(sent_dove);
}

bool TimeGraphArk::SendData( fluctus::DataInfo const & data_info)
{
    return false;
}



