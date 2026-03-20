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