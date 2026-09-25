#pragma once
#include "ui_spectral_viewer_window.h"
#include <qdialog.h>
#include <qpropertyanimation.h>
#include <qbuttongroup.h>
#include "Arks/Interfaces/ark_interface.h"

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
        void AddWindow(ChartType window_type, QWidget* widget_ptr, ArkInterface* ark_ptr);
        void SetMaxFFtOrder(int n_fft_order);
        void ActivateCur(bool do_activate);

    signals:
        void FftChangeNeed(int new_fft);
        void RecordSelectionNeed();

    protected:
        bool eventFilter(QObject* obj, QEvent* event) override;
        void UpdateFFtCombobox(int max_order, int cur_fft_order);
        void SetupSideMenu();
        void AnimateWidth(int target_width);

    private:
        struct WidgetEntry {
            QWidget* widget = nullptr;
            ArkInterface* ark = nullptr;
        };

        void SetupButtons();
        void SetMenuExpanded(bool expanded);
        void SwitchTo(ChartType type);

        static constexpr int kCollapsedWidth = 40;
        static constexpr int kExpandedWidth = 120;

        Ui::spectral_viewer_window ui_;

        std::map<ChartType, WidgetEntry> chart_entries_;

        QButtonGroup* side_buttons_ = nullptr;
        // Какая кнопка соответствует какому типу — нужно для SwitchTo() и
        // чтобы подсветить активную вкладку.
        std::map<ChartType, QPushButton*> buttons_;

        QWidget* active_down_widget_ = nullptr;

        bool side_menu_expanded_ = false;
        bool hover_expand_active_ = false;

        QTimer* hover_timer_ = nullptr;
        QPropertyAnimation* max_width_anim_ = nullptr;
        QPropertyAnimation* min_width_anim_ = nullptr;
    };
}