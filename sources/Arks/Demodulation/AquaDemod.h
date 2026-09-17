#pragma once
#include <qpointer.h>
#include "Arks/Interfaces/base_impl/ark_base.h"
#include "window/AquaDemodWindow.h"
using namespace fluctus;
namespace demodulation{
	class AquaDemod : public fluctus::ArkBase
	{
		Q_OBJECT
	public:
		AquaDemod();
		~AquaDemod();
		virtual bool SendData(fluctus::DataInfo const& data_info) override;
		virtual bool PostDove(fluctus::DoveSptr const & sent_dove) override;
		ArkType      GetArkType() const override;
	protected:
		bool Reload();

	protected:
		SourceArk									src_info_;
		QPointer<AquaDemodWindow>				window_;

	};
}

