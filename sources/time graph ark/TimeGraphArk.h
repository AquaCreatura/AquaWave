
#include <qdialog.h>



#include "..\interfaces\base_impl\ark_base.h"
#include "..\interfaces\ark_interface.h"


namespace time_graph
{
    //declare to use as the member field
    class TimeGraphViewer;

    //Add reactions on requests!!! (+Resampler interface)

    class TimeGraphArk : public fluctus::ArkBase
    {
    public:
        TimeGraphArk();
        ~TimeGraphArk();
        //Main function
        virtual bool SendData(fluctus::DataInfo const & data_info) override;
        virtual bool SendDove(fluctus::DoveSptr const & sent_dove) override;
    private:

        //bool AddCyclicReader   ();
    protected:
        std::shared_ptr<TimeGraphViewer> dialog_;
    };

   
    class TimeGraphViewer : public QWidget
    {



    };

};