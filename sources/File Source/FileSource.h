#include <qdialog.h>

#include "ui_file_settings_dialog.h"

#include "FileDataManager.h"
#include "..\interfaces\base_impl\ark_base.h"
#include "..\interfaces\ark_interface.h"

namespace file_source
{
    // Forward declaration to use FileSourceDialog as a member
    class FileSourceDialog;

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

    // Dialog for file source settings
    class FileSourceDialog : public QDialog
    {
    Q_OBJECT
    public:
        FileSourceDialog(); // Constructor
        ~FileSourceDialog();
        // Returns selected file parameters
        const file_params& GetFileInfo() const;

    signals:
        // Signal to update the source when parameters change
        void UpdateSourceNeed();

    protected:
        // Slot to handle file path selection
        void OnChooseFilePath();

        void SetFileName(const QString& file_name);

        // Slot to update data type options in UI
        void UpdateDataTypes();

        // Slot triggered when data type selection changes
        void OnDataTypeChanged();

        // Slot for OK button logic
        void OnOkButton();

    protected:
        Ui::FileSettingsDialog  ui_;        // UI components from .ui file
        file_params             file_info_; // Current file parameters
    };
};
