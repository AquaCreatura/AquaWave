#pragma once
#include <qpointer.h>
#include "DSP Tools/Pipelines/BasePipes.h"
#include "Arks/Interfaces/base_impl/ark_base.h"
#include "GUI/Charts/Constel/ConstelChart.h"
#include "special_defs/spectral_viewer_defs.h"
namespace constel
{

	class Constellation : public fluctus::ArkBase
	{

	public:
		Constellation(QWidget *parrent = nullptr);
		~Constellation();
		virtual bool SendData(fluctus::DataInfo const& data_info) override;
		virtual bool SendDove(fluctus::DoveSptr const & sent_dove) override;
		ArkType GetArkType() const override;
	protected:
		bool Reload();
	protected:
		QPointer<ChartConstel>		constel_drawer_;
		SourceArk                  src_info_;
		pipes::SimplePipeLine		pipe_line_;
	};



}