#pragma once
#include <qpointer.h>
#include "Arks/Interfaces/base_impl/ark_base.h"

#include "Window/spectral_viewer_window.h"
#include "special_defs/spectral_viewer_defs.h"
#include "Arks/Scope Analyzer/ScopeAnalyzer.h"

namespace spectral_viewer {
	class SpectralViewer : public fluctus::ArkBase
	{
		Q_OBJECT
	public:
		SpectralViewer();
		~SpectralViewer();
		virtual bool SendData(fluctus::DataInfo const& data_info) override;
		virtual bool PostDove(fluctus::DoveSptr const & sent_dove) override;
		ArkType      GetArkType() const override;
	protected:
		bool Reload();
		void SetNewFftOrder(int n_fft_order);
		void StartSelectionRecord();
		void OnSelectionIsReady();

	protected:
		SourceArk									src_info_;
		QPointer<SpectralViewerWindow>				window_;
		std::shared_ptr<fluctus::ArkInterface>      spectrum_;
		std::shared_ptr<fluctus::ArkInterface>		spg_;
		std::shared_ptr<fluctus::ArkInterface>		scoper_;
		std::shared_ptr<aqua_gui::SelectionHolder>  selection_holder_;
		std::atomic_int64_t							n_fft_{ -1 };
	};
}

