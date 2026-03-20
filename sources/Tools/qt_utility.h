#pragma once
#include <QTimer>
#include <functional>

namespace utility_aqua
{
	class DelayedCaller : public QObject
	{
		Q_OBJECT
	public:
		explicit DelayedCaller(int stab_time_msec,
			bool update_timer = true,
			QObject* parent = nullptr);

		void Call(std::function<void()>&& func);

		private slots:
		void Exec();

	private:
		QTimer m_timer;
		std::function<void()> m_func;
		bool m_update_timer;
	};
};