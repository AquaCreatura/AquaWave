#include "AquaDemod.h"

#include "Interfaces/special_defs/demodulator_defs.h"
#include "Interfaces/special_defs/file_souce_defs.h"

#include <qmessagebox.h>

#include <algorithm>
#include <cmath>

using namespace aqua;

namespace
{
	constexpr double kTwoPi = 2.0 * M_PI;
	// Размер блока, который источник отдаёт демодулятору за раз.
	constexpr int kDemodChunkSize = 4096;
}


demodulator::AquaDemod::AquaDemod()
{
	window_ = new AquaDemodWindow;

	window_->SetSpeedRanges(
		demodulator_.GetPllSpeedLimits(),
		demodulator_.GetTimingSpeedLimits(),
		demodulator_.GetEqualiserSpeedLimits(),
		demodulator_.GetPllSpeedDefault(),
		demodulator_.GetTimingSpeedDefault(),
		demodulator_.GetEqualiserSpeedDefault());
	window_->SetRunningState(false);

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
	connect(window_, &AquaDemodWindow::RunToggled, this, [this](bool running) {
		if (running) {
			// Старт демодуляции: просим источник отдать выделенный диапазон.
			if (!StartDemodulation()) window_->SetRunningState(false);
		}
		else {
			StopDemodulation();
		}
	});

	info_timer_.setInterval(200);
	info_timer_.callOnTimeout(this, &AquaDemod::UpdateEstimates);
}

demodulator::AquaDemod::~AquaDemod()
{
	info_timer_.stop();
	StopDemodulation();
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
	// Демодуляция управляется кнопкой Start/Stop, а не активацией вкладки:
	// kActivate/kDeactivate только показывают/скрывают окно и не трогают поток.
	if (base_thought & aqua::DoveParrent::DoveThought::kSpecialThought) {
		const auto special_thought = sent_dove->special_thought;
		if (auto demod_dove = std::dynamic_pointer_cast<demodulator::DemodulatorDove>(sent_dove)) {
			if (special_thought & demodulator::DemodulatorDove::kInitialise && demod_dove->carrier_hz && demod_dove->symbol_rate_hz) {
				// Параметры из гармонического анализатора применяются только
				// до запуска: во время демодуляции управляет кнопка Start/Stop.
				if (!is_active_) {
					window_->SetFreqParams(demod_dove->carrier_hz, demod_dove->symbol_rate_hz);
					ApplyDemodSettings();
				}
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


	auto qstr_mod = window_->GetModulation().toUtf8();
	modulation_ = std::string(qstr_mod.constData(), qstr_mod.size());
	carrier_hz_ = window_->GetCarrierHz();
	symbol_rate_hz_ = window_->GetSymbolRateHz();

	if (symbol_rate_hz_ <= 0.0) return;

	if (!demodulator_.Init(modulation_.c_str(), upsample_koeff_)) return;
	demodulator_.SetPllSpeed(window_->GetPllSpeed());
	demodulator_.SetTimingSpeed(window_->GetTllSpeed());
	demodulator_.SetEqualiserSpeed(window_->GetEqSpeed());

	// Источник по запросу StartDemodulation отдаёт выделенную полосу, уже
	// приведённую к нулевой несущей, с ЧД symbol_rate * upsample. Поэтому
	// тракту достаточно убрать постоянную составляющую перед демодулятором.
	pipe_line_.AddNextPipe(std::make_shared<pipes::DcRemovePipe>());

	is_ready_ = true;
}

bool demodulator::AquaDemod::StartDemodulation()
{
	auto source_ark = src_info_.ark.lock();
	if (!source_ark) return false;

	// Пересобираем тракт под текущие параметры окна и убеждаемся,
	// что демодулятор инициализирован (нужны несущая и символьная скорость).
	ApplyDemodSettings();
	bool ready = false;
	{
		tbb::spin_mutex::scoped_lock scoped_locker(pipe_mutex_);
		ready = is_ready_;
		if (ready) synced_buffer_.clear();
	}
	if (!ready) {
		QMessageBox::warning(nullptr, "Cannot Start Demodulation",
			"Set the carrier, symbol rate and modulation first.");
		return false;
	}
	if (window_) window_->ClearConstelData();

	// Аналогично ScopeAnalyser: просим источник начать циклическую отдачу
	// выделенного диапазона. Полоса берётся по символьной скорости, а ЧД —
	// так, чтобы демодулятор получал upsample отсчётов на символ.
	self_wptr_ = shared_from_this();
	auto req_dove = std::make_shared<file_source::FileSrcDove>(
		file_source::FileSrcDove::kInitiate | file_source::FileSrcDove::kAskLoopInRange);
	req_dove->target_ark = self_wptr_.lock();
	req_dove->time_bounds = { 0., 1. };
	req_dove->setup.emplace();
	auto& setup = req_dove->setup;
	setup->carrier_hz    = static_cast<int64_t>(std::llround(carrier_hz_));
	setup->samplerate_hz = static_cast<int64_t>(std::llround(symbol_rate_hz_ * upsample_koeff_));
	setup->banwidth_hz   = static_cast<int64_t>(std::llround(symbol_rate_hz_));
	setup->chunk_size    = kDemodChunkSize;

	// Включаем приём до старта читателя, чтобы не потерять первые блоки.
	is_active_ = true;
	info_timer_.start();
	if (!source_ark->PostDove(req_dove)) {
		is_active_ = false;
		info_timer_.stop();
		QMessageBox::warning(nullptr, "Cannot Start Demodulation",
			"Check the source and demodulation parameters (carrier, symbol rate).");
		return false;
	}
	return true;
}

void demodulator::AquaDemod::StopDemodulation()
{
	is_active_ = false;
	info_timer_.stop();

	// Останавливаем только свой читатель у источника. Во время разрушения
	// объекта self_wptr_ уже истёк, поэтому обращение к shared_from_this()
	// здесь не используется.
	if (auto self = self_wptr_.lock()) {
		if (auto source_ark = src_info_.ark.lock()) {
			auto req_dove = std::make_shared<aqua::DoveParrent>(aqua::DoveParrent::kStop);
			req_dove->target_ark = self;
			source_ark->PostDove(req_dove);
		}
	}

	if (window_) window_->ClearConstelData();
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
