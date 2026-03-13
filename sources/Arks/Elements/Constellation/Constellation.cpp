#include "Constellation.h"
using namespace constel;
using namespace fluctus;
constel::Constellation::Constellation(QWidget * parrent):
	constel_drawer_(new ChartConstel())
{
	
}

constel::Constellation::~Constellation()
{
}

bool constel::Constellation::SendData(fluctus::DataInfo const & data_info)
{
	auto casted_vec = (std::vector<Ipp32fc> &)data_info.data_vec;
	constel_drawer_->PushData(casted_vec);
	return false;
}

bool constel::Constellation::SendDove(fluctus::DoveSptr const & sent_dove)
{
	// Если сообщение недействительно, выбрасываем исключение.
	if (!sent_dove) throw std::invalid_argument("Not created message sent!");

	// Получаем целевое значение и "мысль" из сообщения.
	auto target_val = sent_dove->target_ark;
	auto base_thought = sent_dove->base_thought;

	// Если "мысль" - запрос на диалог.
	if (base_thought & fluctus::DoveParrent::DoveThought::kGetDialog)
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
				
			}
		};
	}
	// Передаём сообщение базовому классу для дальнейшей обработки.
	return ArkBase::SendDove(sent_dove);
	
}

ArkType constel::Constellation::GetArkType() const
{
	return ArkType::kConstellation;
}

bool constel::Constellation::Reload()
{
	return false;
}
