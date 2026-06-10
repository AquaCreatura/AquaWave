#include "window/selection_writer_window.h"
#include "Arks/Interfaces/base_impl/ark_base.h"
using namespace fluctus;
namespace file_writer
{
	// Ark implementation for sending data from a file source
	class SelectionWriter : public fluctus::ArkBase
	{
	public:
		SelectionWriter();  // Constructor
		~SelectionWriter(); // Destructor

						  // Sends data information to the destination
		virtual bool SendData(fluctus::DataInfo const & data_info) override;

		// Sends a Dove object (e.g., command or signal)
		virtual bool PostDove(fluctus::DoveSptr const & sent_dove) override;
		fluctus::ArkType GetArkType() const override;
	protected:
		void UpdateSource();
		bool StartSelectionRecord(fluctus::Limits<double> freq_bounds_hz, fluctus::Limits<double> time_bounds);
	protected:
		SourceArk	src_info_;
		SelectionWriterWindow window_;
	};
};