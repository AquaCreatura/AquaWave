#include "spectral_viewer_window.h"
#include "Utilities/parse_tools.h"
#include <qshortcut.h>
#include <qabstractanimation.h>
#include <qtimer.h>
#include "GUI/Charts/ChartInterface.h"

using namespace spectral_viewer;

SpectralViewerWindow::SpectralViewerWindow()
{
    ui_.setupUi(this);
    SetupSideMenu();
    SetupButtons();

    connect(ui_.fft_order_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [this](int index) {
            emit FftChangeNeed(ui_.fft_order_combobox->itemData(index).toInt());
        });

    // Смена активной нижней вкладки — рассылаем PostDove.
    connect(ui_.main_down_part, &QStackedWidget::currentChanged, this, [this](int) {
        QWidget* curr = ui_.main_down_part->currentWidget();
        if (curr == active_down_widget_)
            return;

        for (auto& [type, entry] : chart_entries_) {
            if (!entry.ark)
                continue;
            if (entry.widget == curr)
                entry.ark->PostDove(std::make_shared<fluctus::DoveParrent>(
                    fluctus::DoveParrent::kActivate));
            else if (entry.widget == active_down_widget_)
                entry.ark->PostDove(std::make_shared<fluctus::DoveParrent>(
                    fluctus::DoveParrent::kDeactivate));
        }
        active_down_widget_ = curr;
        });

    auto* saveShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_S), this);
    connect(saveShortcut, &QShortcut::activated, this, &SpectralViewerWindow::RecordSelectionNeed);

    QTimer::singleShot(0, this, [this]() {
        const int totalHeight = ui_.main_splitter->height();
        if (totalHeight > 0)
            ui_.main_splitter->setSizes({ totalHeight / 2, totalHeight / 2 });
        });

    UpdateFFtCombobox(21, 10);
}

// ---------------------------------------------------------------------------
//  Кнопки нижней панели
// ---------------------------------------------------------------------------

void SpectralViewerWindow::SetupButtons()
{
    side_buttons_ = new QButtonGroup(this);
    side_buttons_->setExclusive(true);

    buttons_[kStaticSpg] = ui_.spectrgoram_side_pushbutton;
    buttons_[kAnalyzer] = ui_.analysis_side_pushbutton;

    for (auto& [type, button] : buttons_)
        side_buttons_->addButton(button);

    connect(side_buttons_, QOverload<int>::of(&QButtonGroup::buttonClicked), this, [this](int id) {
            SwitchTo(static_cast<ChartType>(id));
    });

    // id в группе = значение ChartType — так проще связать клик с типом.
    for (auto& [type, button] : buttons_)
        side_buttons_->setId(button, static_cast<int>(type));

    ui_.spectrgoram_side_pushbutton->setChecked(true);
}

void SpectralViewerWindow::SwitchTo(ChartType type)
{
    auto it = chart_entries_.find(type);
    if (it == chart_entries_.end() || !it->second.widget)
        return;

    ui_.main_down_part->setCurrentWidget(it->second.widget);

    if (auto bit = buttons_.find(type); bit != buttons_.end())
        bit->second->setChecked(true);
}

// ---------------------------------------------------------------------------
//  Боковое меню
// ---------------------------------------------------------------------------

void SpectralViewerWindow::SetupSideMenu()
{
    ui_.extend_frame_button->setIcon(QIcon(":/buttons/button_images/horizontal_extended.png"));
    ui_.extend_frame_button->setIconSize(QSize(20, 20));

    ui_.spectrgoram_side_pushbutton->setIcon(QIcon(":/buttons/button_images/waterfall.png"));
    ui_.spectrgoram_side_pushbutton->setIconSize(QSize(20, 20));

    ui_.analysis_side_pushbutton->setIcon(QIcon(":/buttons/button_images/graph.png"));
    ui_.analysis_side_pushbutton->setIconSize(QSize(20, 20));

    ui_.verticalFrame->installEventFilter(this);

    hover_timer_ = new QTimer(this);
    hover_timer_->setSingleShot(true);
    hover_timer_->setInterval(300);
    connect(hover_timer_, &QTimer::timeout, this, [this]() {
        if (!side_menu_expanded_) {
            hover_expand_active_ = true;
            SetMenuExpanded(true);
        }
        });

    connect(ui_.extend_frame_button, &QPushButton::clicked, this, [this]() {
        hover_expand_active_ = false;
        SetMenuExpanded(ui_.extend_frame_button->isChecked());
        });

    // Стартуем свёрнутыми без анимации.
    ui_.verticalFrame->setMinimumWidth(kCollapsedWidth);
    ui_.verticalFrame->setMaximumWidth(kCollapsedWidth);
    ui_.spectrgoram_side_pushbutton->setText(QString());
    ui_.analysis_side_pushbutton->setText(QString());
}

void SpectralViewerWindow::SetMenuExpanded(bool expanded)
{
    side_menu_expanded_ = expanded;
    AnimateWidth(expanded ? kExpandedWidth : kCollapsedWidth);

    ui_.spectrgoram_side_pushbutton->setText(expanded ? tr("Spectrogram") : QString());
    ui_.analysis_side_pushbutton->setText(expanded ? tr("Analysis") : QString());
}

void SpectralViewerWindow::AnimateWidth(int target_width)
{
    if (!max_width_anim_) {
        max_width_anim_ = new QPropertyAnimation(ui_.verticalFrame, "maximumWidth", this);
        max_width_anim_->setDuration(200);

        min_width_anim_ = new QPropertyAnimation(ui_.verticalFrame, "minimumWidth", this);
        min_width_anim_->setDuration(200);
    }

    auto restart = [](QPropertyAnimation* anim, int from, int to) {
        anim->stop();
        anim->setStartValue(from);
        anim->setEndValue(to);
        anim->start();
    };

    restart(max_width_anim_, ui_.verticalFrame->maximumWidth(), target_width);
    restart(min_width_anim_, ui_.verticalFrame->minimumWidth(), target_width);
}

bool SpectralViewerWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == ui_.verticalFrame) {
        if (event->type() == QEvent::Enter) {
            if (!side_menu_expanded_ && !hover_expand_active_ &&
                !ui_.extend_frame_button->isChecked()) {
                hover_timer_->start();
            }
            return true;
        }
        if (event->type() == QEvent::Leave) {
            hover_timer_->stop();
            if (hover_expand_active_ && !ui_.extend_frame_button->isChecked()) {
                hover_expand_active_ = false;
                SetMenuExpanded(false);
            }
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

// ---------------------------------------------------------------------------
//  Добавление окон
// ---------------------------------------------------------------------------

void SpectralViewerWindow::AddWindow(ChartType window_type, QWidget* widget_ptr, ArkInterface* ark_ptr)
{
    chart_entries_[window_type] = WidgetEntry{ widget_ptr, ark_ptr };

    switch (window_type) {
    case kDpxSpectrum:
        ui_.main_up_part->layout()->addWidget(widget_ptr);
        qobject_cast<ChartInterface*>(widget_ptr)->SetControlButtons(ui_.ctrl_buttons_frame);
        break;

    case kStaticSpg:
    case kAnalyzer:
        ui_.main_down_part->addWidget(widget_ptr);
        break;

    default:
        break;
    }
}

// ---------------------------------------------------------------------------
//  FFT
// ---------------------------------------------------------------------------

void SpectralViewerWindow::SetMaxFFtOrder(int max_fft_order)
{
    const int cur_fft = ui_.fft_order_combobox->currentData().toInt();
    UpdateFFtCombobox(max_fft_order, cur_fft);
}

void SpectralViewerWindow::ActivateCur(bool /*do_activate*/)
{
}

void SpectralViewerWindow::UpdateFFtCombobox(const int max_order, const int cur_fft_order)
{
    {
        QSignalBlocker blocker(ui_.fft_order_combobox);
        ui_.fft_order_combobox->clear();
        for (int fft_counter = 4; fft_counter <= max_order; ++fft_counter) {
            const QString text = QString(
                aqua_parse_tools::ValueToString(1 << fft_counter, 0, " ").c_str());
            ui_.fft_order_combobox->addItem(text, fft_counter);
        }
    }

    const int index = ui_.fft_order_combobox->findData(cur_fft_order);
    if (index != -1) {
        {
            QSignalBlocker blocker(ui_.fft_order_combobox);
            ui_.fft_order_combobox->setCurrentIndex(index);
        }
        emit ui_.fft_order_combobox->currentIndexChanged(index);
    }
}