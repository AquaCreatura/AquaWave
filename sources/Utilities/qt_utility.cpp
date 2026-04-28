#include "qt_utility.h"
using namespace utility_aqua;


DelayedCaller::DelayedCaller(int stab_time_msec,
	bool update_timer,
	QObject* parent)
	: QObject(parent)
	, m_update_timer(update_timer)
{
	m_timer.setSingleShot(true);
	m_timer.setInterval(stab_time_msec);

	connect(&m_timer, &QTimer::timeout, this, &DelayedCaller::Exec);
}

void DelayedCaller::Call(std::function<void()>&& func)
{
	m_func = std::move(func);

	if (!m_timer.isActive() || m_update_timer)
		m_timer.start();
}

void DelayedCaller::Exec()
{
	if (m_func)
	{
		auto f = std::move(m_func);
		m_func = nullptr;
		f();
	}
}
QString buildButtonStyleSheet(
	int size,
	const QString& imgChecked,
	const QString& imgCheckedHover,
	const QString& imgUnchecked,
	const QString& imgUncheckedHover,
	const QString& imgDisabled
)
{
	return QString(R"(
        QPushButton {
            min-width: %1px;
            max-width: %1px;
            min-height: %1px;
            max-height: %1px;
            border: none;
            padding: 0px;
            margin: 0px;
            border-image: url(%2) 0 0 0 0 stretch stretch;
        }

        QPushButton:hover {
            border-image: url(%3) 0 0 0 0 stretch stretch;
        }

        QPushButton:checked {
            border-image: url(%4) 0 0 0 0 stretch stretch;
        }

        QPushButton:checked:hover {
            border-image: url(%5) 0 0 0 0 stretch stretch;
        }

        QPushButton:disabled {
            border-image: url(%6) 0 0 0 0 stretch stretch;
        }
    )")
		.arg(size)
		.arg(imgUnchecked)
		.arg(imgUncheckedHover)
		.arg(imgChecked)
		.arg(imgCheckedHover)
		.arg(imgDisabled);
}

#include <QIcon>
#include <QPushButton>

QIcon buildButtonIcon(
	const QString& imgChecked,
	const QString& imgCheckedHover,
	const QString& imgUnchecked,
	const QString& imgUncheckedHover,
	const QString& imgDisabled
)
{
	QIcon icon;

	// unchecked
	icon.addPixmap(QPixmap(imgUnchecked), QIcon::Normal, QIcon::Off);

	// hover (Qt считает это Active)
	icon.addPixmap(QPixmap(imgUncheckedHover), QIcon::Active, QIcon::Off);

	// checked
	icon.addPixmap(QPixmap(imgChecked), QIcon::Normal, QIcon::On);

	// checked + hover
	icon.addPixmap(QPixmap(imgCheckedHover), QIcon::Active, QIcon::On);

	// disabled (для обоих состояний)
	icon.addPixmap(QPixmap(imgDisabled), QIcon::Disabled, QIcon::Off);
	icon.addPixmap(QPixmap(imgDisabled), QIcon::Disabled, QIcon::On);

	return icon;
}