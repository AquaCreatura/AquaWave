#include <qdialog.h>



#include "helpers/FileDataManager.h"
#include "Arks/Interfaces/base_impl/ark_base.h"
#include "window/FileSourceDialog.h"

namespace file_source
{
    // Ark implementation for sending data from a file source
    class FileSourceArk : public aqua::ArkBase
    {
    public:
        FileSourceArk(QWidget *main_window = nullptr);  // Constructor
        ~FileSourceArk(); // Destructor

        // Sends data information to the destination
        virtual bool SendData(aqua::DataInfo const & data_info) override;

        // Sends a Dove object (e.g., command or signal)
        virtual bool PostDove(aqua::DoveSptr const & sent_dove) override;
        aqua::ArkType GetArkType() const override;
    protected:
        void UpdateSource(); 
    protected:
        FileDataManager                     listener_man_; // Manages file listeners
        QPointer<FileSourceDialog>			dialog_;        // Settings dialog interface
        SourceDescription                         descr_;     // File parameters
		QWidget *							qmain_window_ = nullptr; //Pointer to main to change tittles
    };
};
