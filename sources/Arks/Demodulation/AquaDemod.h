#pragma once
#include "Arks/Interfaces/base_impl/ark_base.h"
#include "DSP Tools/Modem/Demodulation/ContinuousDemodulator.h"
#include "window/AquaDemodWindow.h"

#include <qpointer.h>
#include <qtimer.h>
#include <tbb/spin_mutex.h>

#include <memory>
#include <string>

using namespace aqua;
namespace demodulator{
	class AquaDemod : public aqua::ArkBase
	{
		Q_OBJECT
	public:
		// is_light_mode передаётся в окно: компактное представление для
		// встраивания (например, в ScopeAnalyser).
		AquaDemod(bool is_light_mode = false);
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

		// Запуск/остановка демодуляции: запрос к источнику выделенного
		// диапазона (аналогично ScopeAnalyser) и приём потока данных.
		bool StartDemodulation();
		void StopDemodulation();

	protected:
		SourceArk							src_info_;
		QPointer<AquaDemodWindow>			window_;

		aq_demod::ContinuousDemodulator		demodulator_;
		tbb::spin_mutex						pipe_mutex_;
		QTimer								info_timer_;

		std::vector<Ipp32fc>				synced_buffer_;
		// Слабая ссылка на себя: нужна, чтобы останавливать читатель источника
		// в том числе из деструктора (shared_from_this() там уже недопустим).
		std::weak_ptr<aqua::ArkInterface>	self_wptr_;

		std::string							modulation_{ "QPSK" };
		double								carrier_hz_{ 0.0 };
		double								symbol_rate_hz_{ 0.0 };
		// Диапазон времени для чтения при старте (доли файла).
		aqua::Limits<double>				start_time_bounds_{ 0.9, 1.0 };
		int									upsample_koeff_{ 4 };
		bool								is_active_{ false };
		bool								is_ready_{ false };

	};
}
