#include "AquaDemod.h"

#include "Interfaces/special_defs/demodulator_defs.h"

#include <algorithm>
#include <cmath>

using namespace aqua;

namespace
{
	constexpr double kTwoPi = 2.0 * M_PI;
}


demodulator::AquaDemod::AquaDemod()
{
	window_ = new AquaDemodWindow;

	connect(window_, &AquaDemodWindow::DemodSettingsChanged, this, &AquaDemod::ApplyDemodSettings);
	connect(window_, &AquaDemodWindow::PllSpeedChanged, this, [this](double speed) {
		tbb::spin_mutex::scoped_lock scoped_locker(pipe_mutex_);
		demodulator_.SetPllSpeed(speed);
	});
	connect(window_, &AquaDemodWindow::TllSpeedChanged, this, [this](double speed) {
		tbb::spin_mutex::scoped_lock scoped_locker(pipe_mutex_);
		demodulator_.SetTimingSpeed(speed);
	});
	connect(window_, &AquaDemodWindow::EqSpeedChanged, this, [this](double speed) {
		tbb::spin_mutex::scoped_lock scoped_locker(pipe_mutex_);
		demodulator_.SetEqualiserSpeed(speed);
	});

	info_timer_.setInterval(200);
	info_timer_.callOnTimeout(this, &AquaDemod::UpdateEstimates);
}

demodulator::AquaDemod::~AquaDemod()
{
	info_timer_.stop();
}

bool demodulator::AquaDemod::SendData(aqua::DataInfo const & data_info)
{
	if (!is_active_) return false;
	if (data_info.data_vec.empty()) return true;

	tbb::spin_mutex::scoped_lock scoped_locker(pipe_mutex_);
	if (!is_ready_ || pipe_line_.pipes.empty()) return false;

	auto& passed_iq = (std::vector<Ipp32fc>&)data_info.data_vec;
	pipe_line_.Process(passed_iq);
	auto& resampled_iq = pipe_line_.meta->complex_float_data;
	if (resampled_iq.empty()) return true;

	demodulator_.SynchroniseIQ(resampled_iq, synced_buffer_);
	if (synced_buffer_.empty()) return true;

	if (window_) window_->PushConstelData(synced_buffer_);
	return true;
}

bool demodulator::AquaDemod::PostDove(aqua::DoveSptr const & sent_dove)
{
	// Получаем целевое значение и "мысль" из сообщения.
	if (!sent_dove) throw std::invalid_argument("Not created message sent!");
	auto target_val = sent_dove->target_ark;
	auto base_thought = sent_dove->base_thought;

	if (base_thought & aqua::DoveParrent::DoveThought::kGetWindow)
	{
		sent_dove->show_widget = window_;
		return true; // Запрос обработан.
	}
	if (base_thought & aqua::DoveParrent::DoveThought::kAddSource)
	{
		src_info_.ark = target_val;
		Reload();
	}
	if (base_thought & aqua::DoveParrent::DoveThought::kReset)
	{
		return Reload();
	}
	if (base_thought & aqua::DoveParrent::DoveThought::kActivate)
	{
		is_active_ = true;
		info_timer_.start();
	}
	if (base_thought & aqua::DoveParrent::DoveThought::kDeactivate)
	{
		is_active_ = false;
		info_timer_.stop();
	}
	if (base_thought & aqua::DoveParrent::DoveThought::kSpecialThought) {
		const auto special_thought = sent_dove->special_thought;
		if (auto demod_dove = std::dynamic_pointer_cast<demodulator::DemodulatorDove>(sent_dove)) {
			if (special_thought & demodulator::DemodulatorDove::kInitialise && demod_dove->carrier_hz && demod_dove->symbol_rate_hz) {
				window_->SetFreqParams(demod_dove->carrier_hz, demod_dove->symbol_rate_hz);
				ApplyDemodSettings();
			}
		};
	}
	// Передаём сообщение базовому классу для дальнейшей обработки.
	return ArkBase::PostDove(sent_dove);
}

ArkType demodulator::AquaDemod::GetArkType() const
{
	return ArkType::kCcmDemodulator;
}

bool demodulator::AquaDemod::Reload()
{
	auto source_ark = src_info_.ark.lock();
	if (!source_ark) return true;

	auto req_dove = std::make_shared<aqua::DoveParrent>(aqua::DoveParrent::kGetDescription);
	if (!source_ark->PostDove(req_dove) || !req_dove->description) {
		return false;
	}
	src_info_.descr = *req_dove->description;

	ApplyDemodSettings();
	return true;
}

void demodulator::AquaDemod::ApplyDemodSettings()
{
	tbb::spin_mutex::scoped_lock scoped_locker(pipe_mutex_);
	RebuildPipeline();
}

void demodulator::AquaDemod::RebuildPipeline()
{
	is_ready_ = false;
	pipe_line_.pipes.clear();
	if (window_) window_->ClearConstelData();
	if (!window_) return;


	//modulation_ = window_->GetModulation().toStdString();
	carrier_hz_ = window_->GetCarrierHz();
	symbol_rate_hz_ = window_->GetSymbolRateHz();


	if (symbol_rate_hz_ <= 0.0 || src_info_.descr.samplerate_hz <= 0) return;

	if (!demodulator_.Init(modulation_.c_str(), upsample_koeff_)) return;
	demodulator_.SetPllSpeed(window_->GetPllSpeed());
	demodulator_.SetTimingSpeed(window_->GetTllSpeed());
	demodulator_.SetEqualiserSpeed(window_->GetEqSpeed());

	// Демодулятор работает на 4 отсчётах на символ, поэтому вход приводится
	// к (symbol_rate * upsample) с переносом на выбранную несущую.
	pipe_line_.AddNextPipe(std::make_shared<pipes::DcRemovePipe>());
	pipe_line_.AddNextPipe(std::make_shared<pipes::ResamplerPipe>(
		src_info_.descr.carrier_hz,
		src_info_.descr.samplerate_hz,
		static_cast<int64_t>(std::llround(carrier_hz_)),
		static_cast<int64_t>(std::llround(symbol_rate_hz_ * upsample_koeff_)),
		static_cast<int64_t>(std::llround(symbol_rate_hz_))));

	is_ready_ = true;
}

void demodulator::AquaDemod::UpdateEstimates()
{
	tbb::spin_mutex::scoped_lock scoped_locker(pipe_mutex_);
	if (!is_ready_ || !window_) return;

	const double ratio = demodulator_.GetSymbolRateRatio();
	const double symbol_rate_est = (ratio > 1.e-6) ? symbol_rate_hz_ / ratio : symbol_rate_hz_;
	const double freq_offset_hz = demodulator_.GetFreqOffset() / kTwoPi * symbol_rate_hz_;

	window_->UpdateEstimates(demodulator_.GetLastSNR(), freq_offset_hz, symbol_rate_est);
}
