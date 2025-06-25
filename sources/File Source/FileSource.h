#include <qdialog.h>



#include "helpers/FileDataManager.h"
#include "..\interfaces\base_impl\ark_base.h"
#include "window/FileSourceDialog.h"

namespace file_source
{
    // Ark implementation for sending data from a file source
    class FileSourceArk : public fluctus::ArkBase
    {
    public:
        FileSourceArk();  // Constructor
        ~FileSourceArk(); // Destructor

        // Sends data information to the destination
        virtual bool SendData(fluctus::DataInfo const & data_info) override;

        // Sends a Dove object (e.g., command or signal)
        virtual bool SendDove(fluctus::DoveSptr const & sent_dove) override;
        fluctus::ArkType GetArkType() const override;
    private:
        //bool AddCyclicReader(); // Optional: reader for cyclic data
    protected:
        FileDataManager                     listener_man_; // Manages file listeners
        std::shared_ptr<FileSourceDialog>   dialog_;        // Settings dialog interface
        file_params                         file_info_;     // File parameters
    };
};
