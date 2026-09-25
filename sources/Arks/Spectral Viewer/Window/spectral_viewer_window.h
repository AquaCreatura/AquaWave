#pragma once
#include "ui_spectral_viewer_window.h"
#include <qdialog.h>
#include <qpropertyanimation.h>
#include "Arks/Interfaces/ark_interface.h"


//using namespace fluctus;
using namespace fluctus;
namespace spectral_viewer {
	class SpectralViewerWindow : public QDialog
	{
		Q_OBJECT
	public:
		enum ChartType {
			kDpxSpectrum,
			kStaticSpg,
			kAnalyzer,
		};

		SpectralViewerWindow();
		void AddWindow(ChartType window_type, QWidget* wigdet_ptr, ArkInterface* ark_ptr);
		void SetMaxFFtOrder(int n_fft_order);
		void ActivateCur(bool do_activate);
	signals:
		void FftChangeNeed(int new_fft);
		void RecordSelectionNeed();
	protected:
		bool eventFilter(QObject* obj, QEvent* event) override;
		void UpdateFFtCombobox(const int max_order, const int cur_fft_order);
		void SetupSideMenu();
		void AnimateWidth(int target_width);
	protected:
		Ui::spectral_viewer_window ui_;
		std::map<ChartType, QWidget*>	   widgets_;
		int								   last_down_idx_ = -1;
		std::map<ChartType, ArkInterface*> arks_;
		bool side_menu_expanded_ = false;
		bool hover_expand_active_ = false;
		QTimer* hover_timer_ = nullptr;
	};

}