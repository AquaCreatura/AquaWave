#pragma once
#include <qpointer.h>
#include "DSP Tools/Pipelines/BasePipes.h"
#include "Arks/Interfaces/base_impl/ark_base.h"
#include "GUI/Charts/Constel/ConstelChart.h"
#include "special_defs/spectral_viewer_defs.h"
#include <tbb/spin_mutex.h>
namespace constel
{

	class Constellation : public fluctus::ArkBase
	{

	public:
		Constellation(QWidget *parrent = nullptr);
		~Constellation();
		virtual bool SendData(fluctus::DataInfo const& data_info) override;
		virtual bool PostDove(fluctus::DoveSptr const & sent_dove) override;
		ArkType GetArkType() const override;
	protected:
		bool Reload();
		void UpdatePipeline(int64_t new_fc_hz, int64_t new_fs_hz);
	protected:
		QPointer<ChartConstel>		constel_drawer_;
		SourceArk					src_info_;
		pipes::SimplePipeLine		pipe_line_;
		tbb::spin_mutex				pipe_mutex_;
		int64_t						estim_symbol_rate_;
		int64_t						estim_fc_hz_;
	};



}