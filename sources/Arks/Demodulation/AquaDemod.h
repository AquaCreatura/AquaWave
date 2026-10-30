#pragma once
#include "Arks/Interfaces/base_impl/ark_base.h"
#include "DSP Tools/Modem/ContinuousDemodulator.h"
#include "DSP Tools/Pipelines/BasePipes.h"
#include "window/AquaDemodWindow.h"

#include <qpointer.h>
#include <qtimer.h>
#include <tbb/spin_mutex.h>

#include <string>

using namespace aqua;
namespace demodulator{
	class AquaDemod : public aqua::ArkBase
	{
		Q_OBJECT
	public:
		AquaDemod();
		~AquaDemod();
		virtual bool SendData(aqua::DataInfo const& data_info) override;
		virtual bool PostDove(aqua::DoveSptr const & sent_dove) override;
		ArkType      GetArkType() const override;
	protected:
		bool Reload();
		// Пересборка тракта под текущие настройки окна.
		void ApplyDemodSettings();
		void RebuildPipeline();
		// Периодическое обновление оценочных значений в окне (GUI-поток).
		void UpdateEstimates();

	protected:
		SourceArk							src_info_;
		QPointer<AquaDemodWindow>			window_;

		aq_demod::ContinuousDemodulator		demodulator_;
		pipes::SimplePipeLine				pipe_line_;
		tbb::spin_mutex						pipe_mutex_;
		QTimer								info_timer_;

		std::vector<Ipp32fc>				synced_buffer_;

		std::string							modulation_{ "QPSK" };
		double								carrier_hz_{ 0.0 };
		double								symbol_rate_hz_{ 0.0 };
		int									upsample_koeff_{ 4 };
		bool								is_active_{ false };
		bool								is_ready_{ false };

	};
}
