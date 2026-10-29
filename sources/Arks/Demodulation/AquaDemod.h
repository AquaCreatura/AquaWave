#pragma once
#include "Arks/Interfaces/base_impl/ark_base.h"
#include "window/AquaDemodWindow.h"

#include <qpointer.h>

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

	protected:
		SourceArk									src_info_;
		QPointer<AquaDemodWindow>				window_;

	};
}

