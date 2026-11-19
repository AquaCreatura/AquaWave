#include "CopyValueLabel.h"

#include <qapplication.h>
#include <qclipboard.h>
#include <qevent.h>
#include <qfont.h>

namespace
{
	// Сколько держится индикатор "скопировано".
	constexpr int kCopiedIndicatorMsec = 900;

	// U+2713 (галочка) задаём кодом, чтобы не зависеть от кодировки исходника.
	const ushort kCheckMarkCode = 0x2713;

	const char* const kHoverStyle  = "color: #1a73e8;";
	const char* const kCopiedStyle = "color: #1a8f3c;";
}

CopyValueLabel::CopyValueLabel(QWidget* parent) :
	QLabel(parent)
{
	setCursor(Qt::PointingHandCursor);
	setToolTip(tr("Click to copy"));
	setTextFormat(Qt::PlainText);
	setTextInteractionFlags(Qt::NoTextInteraction);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	copied_timer_.setSingleShot(true);
	copied_timer_.setInterval(kCopiedIndicatorMsec);
	connect(&copied_timer_, &QTimer::timeout, this, [this]() {
		just_copied_ = false;
		UpdateAppearance();
	});
}

void CopyValueLabel::SetValue(const QString& value)
{
	value_ = value;
	UpdateAppearance();
}

void CopyValueLabel::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && !value_.isEmpty()) {
		QApplication::clipboard()->setText(value_);
		just_copied_ = true;
		copied_timer_.start();
		UpdateAppearance();
		event->accept();
		return;
	}
	QLabel::mousePressEvent(event);
}

void CopyValueLabel::enterEvent(QEvent* event)
{
	QLabel::enterEvent(event);
	hovered_ = true;
	UpdateAppearance();
}

void CopyValueLabel::leaveEvent(QEvent* event)
{
	QLabel::leaveEvent(event);
	hovered_ = false;
	UpdateAppearance();
}

void CopyValueLabel::UpdateAppearance()
{
	// Подчёркивание при наведении — подсказка, что значение копируется кликом.
	QFont current_font = font();
	current_font.setUnderline(hovered_ && !just_copied_);
	if (current_font != font())
		setFont(current_font);

	const QString style = just_copied_ ? QString::fromLatin1(kCopiedStyle)
		: (hovered_ ? QString::fromLatin1(kHoverStyle) : QString());
	if (style != styleSheet())
		setStyleSheet(style);

	if (just_copied_)
		setText(QString(QChar(kCheckMarkCode)) + ' ' + value_);
	else
		setText(value_);
}
