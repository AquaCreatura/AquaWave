#include "AquaDemod.h"

#include "Interfaces/special_defs/demodulator_defs.h"

demodulator::AquaDemod::AquaDemod()
{
	window_ = new AquaDemodWindow;
}

demodulator::AquaDemod::~AquaDemod()
{
}

bool demodulator::AquaDemod::SendData(aqua::DataInfo const & data_info)
{
	return false;
}

bool demodulator::AquaDemod::PostDove(aqua::DoveSptr const & sent_dove)
{
	// Получаем целевое значение и "мысль" из сообщения.
	auto target_val = sent_dove->target_ark;
	auto base_thought = sent_dove->base_thought;
	if (base_thought & aqua::DoveParrent::DoveThought::kGetWindow)
	{		
		sent_dove->show_widget = window_;
		return true; // Запрос обработан.
	}
	if (base_thought & aqua::DoveParrent::DoveThought::kSpecialThought) {
		const auto special_thought = sent_dove->special_thought;
		if (auto demod_dove = std::dynamic_pointer_cast<demodulator::DemodulatorDove>(sent_dove)) {
			if (special_thought & demodulator::DemodulatorDove::kInitialise  && demod_dove->carrier_hz && demod_dove->symbol_rate_hz) {
				window_->SetFreqParams(demod_dove->carrier_hz, demod_dove->symbol_rate_hz);
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
	return false;
}
