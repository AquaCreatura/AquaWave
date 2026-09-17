#include "AquaDemod.h"

demodulation::AquaDemod::AquaDemod()
{
	window_ = new AquaDemodWindow;
}

demodulation::AquaDemod::~AquaDemod()
{
}

bool demodulation::AquaDemod::SendData(fluctus::DataInfo const & data_info)
{
	return false;
}

bool demodulation::AquaDemod::PostDove(fluctus::DoveSptr const & sent_dove)
{
	// Получаем целевое значение и "мысль" из сообщения.
	auto target_val = sent_dove->target_ark;
	auto base_thought = sent_dove->base_thought;
	if (base_thought & fluctus::DoveParrent::DoveThought::kGetWindow)
	{		
		sent_dove->show_widget = window_;
		return true; // Запрос обработан.
	}
	// Передаём сообщение базовому классу для дальнейшей обработки.
	return ArkBase::PostDove(sent_dove);
}

ArkType demodulation::AquaDemod::GetArkType() const
{
	return ArkType::kBaseDemodulator;
}

bool demodulation::AquaDemod::Reload()
{
	return false;
}
