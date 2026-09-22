#include "QWindowKitHelper.h"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>

QWindowKitHelper::QWindowKitHelper(QMainWindow* window)
    : QObject(window)
    , m_window(window)
{
    m_windowAgent = new QWK::WidgetWindowAgent(window);
    m_windowAgent->setup(window);
}

void QWindowKitHelper::setupTitleBar(QWidget* passed_bar)
{
    // 1. Создаем контейнер для TitleBar
    auto* titleBar = new QWidget(m_window);
    titleBar->setFixedHeight(40);

    // 2. Создаем элементы управления
    auto* iconButton = new QPushButton(QStringLiteral("App"), m_window);


    auto* titleLabel = new QLabel(m_window->windowTitle(), m_window);
    titleLabel->setAlignment(Qt::AlignCenter);

    m_minButton = new QPushButton(QStringLiteral("-"), m_window);
    m_maxButton = new QPushButton(QStringLiteral("O"), m_window);
    m_closeButton = new QPushButton(QStringLiteral("X"), m_window);

    // 3. Настраиваем layout
    auto* layout = new QHBoxLayout(titleBar);
    layout->setContentsMargins(8, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(iconButton);
    layout->addWidget(passed_bar);
    layout->addWidget(titleLabel, 1); // 1 означает, что лейбл будет растягиваться
    layout->addWidget(m_minButton);
    layout->addWidget(m_maxButton);
    layout->addWidget(m_closeButton);

    // 4. Устанавливаем кастомный виджет меню в главное окно
    m_window->setMenuWidget(titleBar);

    // 5. Настраиваем QWindowKit Agent
    m_windowAgent->setTitleBar(titleBar);
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::WindowIcon, iconButton);
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::Minimize, m_minButton);
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::Maximize, m_maxButton);
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::Close, m_closeButton);

    // Разрешаем клики и перетаскивание через менюбар
    m_windowAgent->setHitTestVisible(passed_bar, true);

    // 6. Подключаем сигналы кнопок к слотам окна
    connect(m_minButton, &QPushButton::clicked, m_window, &QWidget::showMinimized);

    connect(m_maxButton, &QPushButton::clicked, m_window, [this]() {
        if (m_window->isMaximized()) {
            m_window->showNormal();
        }
        else {
            m_window->showMaximized();
        }
        });

    connect(m_closeButton, &QPushButton::clicked, m_window, &QWidget::close);
}