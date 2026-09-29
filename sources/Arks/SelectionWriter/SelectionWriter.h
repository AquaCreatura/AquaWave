#include "window/selection_writer_window.h"
#include "Arks/Interfaces/base_impl/ark_base.h"
#include "Utilities/file_helpers.h"
#include <qelapsedtimer.h>
#include <qtimer.h>
using namespace aqua;
namespace file_writer
{
	// Ark implementation for sending data from a file source
	class SelectionWriter : public aqua::ArkBase
	{
	public:
		SelectionWriter();  // Constructor
		~SelectionWriter(); // Destructor

						  // Sends data information to the destination
		virtual bool		SendData(aqua::DataInfo const & data_info) override;

		// Sends a Dove object (e.g., command or signal)
		virtual bool		PostDove(aqua::DoveSptr const & sent_dove) override;
		aqua::ArkType	GetArkType() const override;
	protected:
		void UpdateSource();
		bool InitSelectionRecord(aqua::Limits<double> freq_bounds_hz, aqua::Limits<double> time_bounds);
		bool StartRecording(const std::string folder_path);
		bool CaptureFile(const std::string folder_path);
		bool StopRecording();
		void OnStopTimerEvent();
	protected:
		SourceArk				src_info_;
		SelectionWriterWindow	window_;
		Limits<double>			time_bounds_;
		Limits<double>			selection_bound_hz_;
		int64_t					work_samplerate_hz_;
		FileWriter				writer_;
		std::vector<Ipp16sc>	casted_16sc_;
		std::atomic<bool>		is_started_{false};
		QTimer					stop_timer_;
		QElapsedTimer			status_update_timer_;
		int64_t					interval_stop_msec_{100};
	};
};