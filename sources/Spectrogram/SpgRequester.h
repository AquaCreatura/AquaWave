#pragma once
#include "Interfaces/ark_interface.h"
#include "GUI/Charts/SPG/SPG core/spg_defs.h"
using namespace fluctus;
namespace spg_core
{
//Class, which request data from the file source
class SpgRequester : public QObject
{
Q_OBJECT
public:
    SpgRequester(const spg_data& spg);
    void SetFileSource      (const ArkWptr& file_source);
    void SetArkSpectrogram  (const ArkWptr& ark_spg);
slots
    void RequestData  ();
protected:
    ArkWptr                     ark_spg_;
    ArkWptr                     ark_file_src_;
    const spg_data&             spg_;

};


}