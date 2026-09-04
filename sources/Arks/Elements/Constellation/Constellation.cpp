#include "Constellation.h"
#include "special_defs/analyzer_defs.h"
#include "DSP Tools/Pipelines/DemodPipes.h"
using namespace constel;
using namespace fluctus;
using namespace pipes;

constel::Constellation::Constellation(QWidget * parrent):
	constel_drawer_(new ChartConstel())
{
	
}

constel::Constellation::~Constellation()
{
}

bool constel::Constellation::SendData(fluctus::DataInfo const & data_info)
{
	if (pipe_line_.pipes.empty()) return false;
	auto casted_vec = (std::vector<Ipp32fc> &)data_info.data_vec;
	{
		tbb::spin_mutex::scoped_lock scoped_locker(pipe_mutex_);
		pipe_line_.Process(casted_vec);
		auto synced = pipe_line_.meta->complex_float_data;
		constel_drawer_->PushData(synced);
	}
	return true;
}

bool constel::Constellation::PostDove(fluctus::DoveSptr const & sent_dove)
{
	// Если сообщение недействительно, выбрасываем исключение.
	if (!sent_dove) throw std::invalid_argument("Not created message sent!");

	// Получаем целевое значение и "мысль" из сообщения.
	auto target_val = sent_dove->target_ark;
	auto base_thought = sent_dove->base_thought;

	// Если "мысль" - запрос на диалог.
	if (base_thought & fluctus::DoveParrent::DoveThought::kGetWindow)
	{
		// Прикрепляем отрисовщик спектра к виджету сообщения.
		sent_dove->show_widget = constel_drawer_;
		return true; // Запрос обработан.
	}
	if (base_thought & fluctus::DoveParrent::DoveThought::kTieSource)
	{
		src_info_.ark = target_val;
		Reload();
	}
	if (base_thought & fluctus::DoveParrent::DoveThought::kReset)
	{
		Reload();
	}
	if (base_thought & fluctus::DoveParrent::DoveThought::kSpecialThought) {
		const auto special_thought = sent_dove->special_thought;
		if (auto spectral_dove = std::dynamic_pointer_cast<spectral_viewer::SpectralDove>(sent_dove)) {

			if (special_thought & spectral_viewer::SpectralDove::kSetFFtOrder) {
				Reload();
			}
		};
		if (auto analyze_dove = std::dynamic_pointer_cast<analyzer::AnalyzeDove>(sent_dove)) {
			if (special_thought & analyzer::AnalyzeDove::kGetHarmonicResult) {
				analyze_dove->text_result = "NaN";
			}
			if (special_thought & analyzer::AnalyzeDove::kSetHarmonicInfo && analyze_dove->carrier_hz && analyze_dove->symbol_rate_hz) {
				
				UpdatePipeline(*analyze_dove->carrier_hz, *analyze_dove->symbol_rate_hz);
			}
		};
	}
	// Передаём сообщение базовому классу для дальнейшей обработки.
	return ArkBase::PostDove(sent_dove);
	
}

ArkType constel::Constellation::GetArkType() const
{
	return ArkType::kConstellation;
}

bool constel::Constellation::Reload()
{
	auto file_src = src_info_.ark.lock();
	if (!file_src) return true;

	auto req_dove = std::make_shared<fluctus::DoveParrent>(fluctus::DoveParrent::kGetDescription);
	req_dove->sender = shared_from_this();
	if (!file_src->PostDove(req_dove) || !req_dove->description)
	{
		return false;
	}
	src_info_.descr = *req_dove->description;
	constel_drawer_->ClearData();
	return true;
}

void constel::Constellation::UpdatePipeline(int64_t new_fc_hz, int64_t new_symbol_rate_hz)
{
	double max_sr = std::max(estim_symbol_rate_, new_symbol_rate_hz);
	if ((std::abs(new_fc_hz - estim_fc_hz_) < 1.e-5 * max_sr) &&
		(std::abs(estim_symbol_rate_ - new_symbol_rate_hz) < 1.e-5 * max_sr)) {
		return;
	}

	{ //Для наглядности ограничем scope
		tbb::spin_mutex::scoped_lock scoped_locker(pipe_mutex_);

		estim_symbol_rate_ = new_symbol_rate_hz;
		estim_fc_hz_ = new_fc_hz;

		pipe_line_.pipes.clear();

		pipe_line_.AddNextPipe(std::make_shared<ResamplerPipe>(src_info_.descr.carrier_hz, src_info_.descr.samplerate_hz, estim_fc_hz_, estim_symbol_rate_ * 4, estim_symbol_rate_));
		pipe_line_.AddNextPipe(std::make_shared<CcmSyncer>("QPSK", 4));
	}
	
}