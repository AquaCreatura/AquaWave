#include "spectral_viewer_window.h"
#include "Utilities/parse_tools.h"
#include <qshortcut.h>
#include <qabstractanimation.h>
#include <qtimer.h>
#include "GUI/Charts/ChartInterface.h"
#include "Arks/Interfaces/ark_interface.h"
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

	connect(ui_.main_down_part, &QStackedWidget::currentChanged, this, [this](int) {
		QWidget* current = ui_.main_down_part->currentWidget();
		for (auto& kv : widgets_) {
			auto dove = std::make_shared<fluctus::DoveParrent>(
				(kv.second == current) ? fluctus::DoveParrent::kActivate : fluctus::DoveParrent::kDeactivate);
			((ArkInterface*)kv.second)->PostDove(dove);
		}
	});

	QShortcut* saveShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_S), this);
	connect(saveShortcut, &QShortcut::activated, this, &SpectralViewerWindow::RecordSelectionNeed);

	UpdateFFtCombobox(21, 10);
	ui_.main_splitter->setSizes({ 1,1 });
}

void SpectralViewerWindow::SetupSideMenu()
{
	QIcon menu_icon("D:/PetWave/AquaWave/sources/GUI/button_images/horizontal_extended.png");
	ui_.extend_frame_button->setIcon(menu_icon);
	ui_.extend_frame_button->setIconSize(QSize(20, 20));

	QIcon spectrogram_icon("D:/PetWave/AquaWave/sources/GUI/button_images/spectrum.png");
	ui_.spectrgoram_side_pushbutton->setIcon(spectrogram_icon);
	ui_.spectrgoram_side_pushbutton->setIconSize(QSize(24, 24));

	QIcon analysis_icon("D:/PetWave/AquaWave/sources/GUI/button_images/analyze_icon.png");
	ui_.analysis_side_pushbutton->setIcon(analysis_icon);
	ui_.analysis_side_pushbutton->setIconSize(QSize(24, 24));

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
void spectral_viewer::SpectralViewerWindow::AddWindow(QWidget * wigdet_ptr, ChartType window_type)
{
	widgets_[window_type] = wigdet_ptr;
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






