#pragma once

#include <QObject>
#include <QMenuBar>
#include <QMainWindow>
#include <QPushButton>

#include <QWKWidgets/widgetwindowagent.h>

class QWindowKitHelper : public QObject
{
    Q_OBJECT
public:
    explicit QWindowKitHelper(QMainWindow* window);

    // Убираем параметр QMenuBar*, так как теперь создаем весь TitleBar внутри
    void setupTitleBar(QWidget* passed_bar);

private:
    QMainWindow* m_window;
    QWK::WidgetWindowAgent* m_windowAgent;

    // Сохраняем указатели на кнопки, если потребуется доступ к ним извне
    QPushButton* m_minButton = nullptr;
    QPushButton* m_maxButton = nullptr;
    QPushButton* m_closeButton = nullptr;
};
