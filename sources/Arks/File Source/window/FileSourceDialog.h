#include <qdialog.h>

#include "ui_file_settings_dialog.h"
#include "File Source/file_souce_defs.h"


namespace file_source
{
    // Dialog for file source settings
    class FileSourceDialog : public QDialog
    {
    Q_OBJECT
    public:
        FileSourceDialog(); // Constructor
        ~FileSourceDialog();
        // Returns selected file parameters
        const file_params& GetFileInfo() const;
        const bool         SetFileName(const QString& file_name);
    signals:
        // Signal to update the source when parameters change
        void UpdateSourceNeed();

    protected:
        // Slot to handle file path selection
        void OnChooseFilePath();

        void ParseFileName(const QString& file_name);
		void RememberFilePath();
        // Slot to update data type options in UI
        void UpdateDataTypes();

        // Slot triggered when data type selection changes
        void OnDataTypeChanged();

        // Slot for OK button logic
        void OnOkButton();

    protected:
        Ui::FileSettingsDialog  ui_;			// UI components from .ui file
        file_params             edit_file_info_;		// Current file parameters
		file_params				actual_file_info_;	//
    };
};
