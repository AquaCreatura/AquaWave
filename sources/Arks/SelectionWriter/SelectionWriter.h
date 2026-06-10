#include <qdialog.h>


#include "Arks\Interfaces\base_impl\ark_base.h"

namespace file_source
{
	// Ark implementation for sending data from a file source
	class FileSourceArk : public fluctus::ArkBase
	{
	public:
		FileSourceArk(QWidget *main_window = nullptr);  // Constructor
		~FileSourceArk(); // Destructor

						  // Sends data information to the destination
		virtual bool SendData(fluctus::DataInfo const & data_info) override;

		// Sends a Dove object (e.g., command or signal)
		virtual bool PostDove(fluctus::DoveSptr const & sent_dove) override;
		fluctus::ArkType GetArkType() const override;
	protected:
		void UpdateSource();
	protected:
		QWidget *qmain_window_ = nullptr; //Pointer to main to change tittles
	};
};