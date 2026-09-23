#pragma once
#include "ui_spectral_viewer_window.h"
#include <qdialog.h>
#include <qpropertyanimation.h>



//using namespace fluctus;

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
		void AddWindow(QWidget* wigdet_ptr, ChartType window_type);
		void SetDpxSpectrumWindow(QWidget* wigdet_ptr);
		void SetSpectrogramWindow(QWidget* wigdet_ptr);
		void SetMaxFFtOrder(int n_fft_order);
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
		std::map<ChartType, QWidget*> widgets_;
		bool side_menu_expanded_ = false;
		bool hover_expand_active_ = false;
		QTimer* hover_timer_ = nullptr;
	};

}