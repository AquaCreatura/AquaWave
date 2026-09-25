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
	connect(ui_.fft_order_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
		int fft_id = ui_.fft_order_combobox->itemData(index).toInt();
		emit FftChangeNeed(fft_id);
	});
	connect(ui_.analysis_side_pushbutton, &QPushButton::clicked, [this]() {
		ui_.main_down_part->setCurrentWidget(widgets_[kAnalyzer]);
		ui_.analysis_side_pushbutton->setChecked(true);
		ui_.spectrgoram_side_pushbutton->setChecked(false);
	});
	connect(ui_.spectrgoram_side_pushbutton, &QPushButton::clicked, [this]() {
		ui_.main_down_part->setCurrentWidget(widgets_[kStaticSpg]);
		ui_.spectrgoram_side_pushbutton->setChecked(true);
		ui_.analysis_side_pushbutton->setChecked(false);
	});
	ui_.spectrgoram_side_pushbutton->setChecked(true);

	connect(ui_.main_down_part, &QStackedWidget::currentChanged, this, [this](int idx) {
		QWidget* prev = last_down_idx_ >= 0 ? ui_.main_down_part->widget(last_down_idx_) : nullptr;
		QWidget* curr = ui_.main_down_part->currentWidget();

		for (auto& [key, widget] : widgets_) {
			if (widget == curr)
				arks_[key]->PostDove(std::make_shared<fluctus::DoveParrent>(fluctus::DoveParrent::kActivate));
			else if (widget == prev)
				arks_[key]->PostDove(std::make_shared<fluctus::DoveParrent>(fluctus::DoveParrent::kDeactivate));
		}
		last_down_idx_ = idx;
	});

	QShortcut* saveShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_S), this);
	connect(saveShortcut, &QShortcut::activated, this, &SpectralViewerWindow::RecordSelectionNeed);

	QTimer::singleShot(0, this, [this]() {
		int totalHeight = ui_.main_splitter->height();
		if (totalHeight > 0) { ui_.main_splitter->setSizes({ totalHeight / 2, totalHeight / 2 }); }
	});

	UpdateFFtCombobox(21, 10);

}


void SpectralViewerWindow::SetupSideMenu()
{
	QIcon menu_icon(":/buttons/button_images/horizontal_extended.png");
	ui_.extend_frame_button->setIcon(menu_icon);
	ui_.extend_frame_button->setIconSize(QSize(20, 20));

	QIcon spectrogram_icon(":/buttons/button_images/waterfall.png");
	ui_.spectrgoram_side_pushbutton->setIcon(spectrogram_icon);
	ui_.spectrgoram_side_pushbutton->setIconSize(QSize(20, 20));

	QIcon analysis_icon(":/buttons/button_images/graph.png");
	ui_.analysis_side_pushbutton->setIcon(analysis_icon);
	ui_.analysis_side_pushbutton->setIconSize(QSize(20, 20));

	ui_.verticalFrame->installEventFilter(this);

	hover_timer_ = new QTimer(this);
	hover_timer_->setSingleShot(true);
	hover_timer_->setInterval(300);

	auto expand_menu = [this]() {
		side_menu_expanded_ = true;
		AnimateWidth(120);
		ui_.spectrgoram_side_pushbutton->setText("Spectrogram");
		ui_.analysis_side_pushbutton->setText("Analysis");
	};

	auto collapse_menu = [this]() {
		if (!ui_.extend_frame_button->isChecked()) {
			side_menu_expanded_ = false;
			AnimateWidth(40);
			ui_.spectrgoram_side_pushbutton->setText("");
			ui_.analysis_side_pushbutton->setText("");
		}
	};

	connect(hover_timer_, &QTimer::timeout, this, [expand_menu, this]() {
		if (!side_menu_expanded_ && !hover_expand_active_) {
			hover_expand_active_ = true;
			expand_menu();
		}
	});

	connect(ui_.extend_frame_button, &QPushButton::clicked, this, [expand_menu, this]() {
		hover_expand_active_ = false;
		if (ui_.extend_frame_button->isChecked()) {
			expand_menu();
		} else {
			side_menu_expanded_ = false;
			AnimateWidth(40);
			ui_.spectrgoram_side_pushbutton->setText("");
			ui_.analysis_side_pushbutton->setText("");
		}
	});

	if (!side_menu_expanded_) {
		ui_.verticalFrame->setMaximumWidth(40);
		ui_.verticalFrame->setMinimumWidth(40);
		ui_.spectrgoram_side_pushbutton->setText("");
		ui_.analysis_side_pushbutton->setText("");
	}
}

void SpectralViewerWindow::AnimateWidth(int target_width)
{
	auto* max_anim = new QPropertyAnimation(ui_.verticalFrame, "maximumWidth", this);
	max_anim->setDuration(200);
	max_anim->setStartValue(ui_.verticalFrame->maximumWidth());
	max_anim->setEndValue(target_width);
	max_anim->start(QAbstractAnimation::DeleteWhenStopped);

	auto* min_anim = new QPropertyAnimation(ui_.verticalFrame, "minimumWidth", this);
	min_anim->setDuration(200);
	min_anim->setStartValue(ui_.verticalFrame->minimumWidth());
	min_anim->setEndValue(target_width);
	min_anim->start(QAbstractAnimation::DeleteWhenStopped);
}

bool SpectralViewerWindow::eventFilter(QObject* obj, QEvent* event)
{
	if (obj == ui_.verticalFrame) {
		if (event->type() == QEvent::Enter) {
			if (!side_menu_expanded_ && !hover_expand_active_ && !ui_.extend_frame_button->isChecked()) {
				hover_timer_->start();
			}
			return true;
		} else if (event->type() == QEvent::Leave) {
			hover_timer_->stop();
			if (hover_expand_active_ && !ui_.extend_frame_button->isChecked()) {
				hover_expand_active_ = false;
				side_menu_expanded_ = false;
				AnimateWidth(40);
				ui_.spectrgoram_side_pushbutton->setText("");
				ui_.analysis_side_pushbutton->setText("");
			}
			return true;
		}
	}
	return QDialog::eventFilter(obj, event);
}
void spectral_viewer::SpectralViewerWindow::AddWindow(ChartType window_type, QWidget* wigdet_ptr, ArkInterface* ark_ptr)
{
	widgets_[window_type] = wigdet_ptr;
	arks_[window_type] = ark_ptr;
	switch (window_type)
	{
	case spectral_viewer::SpectralViewerWindow::kDpxSpectrum:
		ui_.main_up_part->layout()->addWidget(wigdet_ptr);
		qobject_cast<ChartInterface*>(wigdet_ptr)->SetControlButtons(ui_.ctrl_buttons_frame);
		break;
	case spectral_viewer::SpectralViewerWindow::kStaticSpg:
	case spectral_viewer::SpectralViewerWindow::kAnalyzer:
		ui_.main_down_part->addWidget(wigdet_ptr);
		break;
	default:
		break;
	}
}

void SpectralViewerWindow::SetMaxFFtOrder(int max_fft_order)
{
	const auto cur_fft = ui_.fft_order_combobox->currentData().toInt();
	UpdateFFtCombobox(max_fft_order, cur_fft);
}
void spectral_viewer::SpectralViewerWindow::ActivateCur(bool do_activate)
{

}
void SpectralViewerWindow::UpdateFFtCombobox(const int max_order, const int cur_fft_order)
{
	{
		QSignalBlocker blocker(ui_.fft_order_combobox);
		ui_.fft_order_combobox->clear();
		for (int fft_counter = 4; fft_counter <= max_order; fft_counter++) {
			QString item_text = QString("%1").arg(aqua_parse_tools::ValueToString(1 << fft_counter, 0, " ").c_str());
			ui_.fft_order_combobox->addItem(item_text, fft_counter);
		}
	}
	{
		int target_fft = cur_fft_order;
		int index = ui_.fft_order_combobox->findData(target_fft);
		if (index != -1) {
			{
				QSignalBlocker blocker(ui_.fft_order_combobox);
				ui_.fft_order_combobox->setCurrentIndex(index);
			}
			emit ui_.fft_order_combobox->currentIndexChanged(index);
		}

	}
}






