#include "QMainMenuHelper.h"

QMainMenuHelper::QMainMenuHelper(QMainWindow* mainWindow)
    : QObject(mainWindow)
    , m_mainWindow(mainWindow)
{
}

QMenuBar* QMainMenuHelper::createMenuBar(QWidget* parent)
{
    m_menuBar = new QMenuBar(parent);
    createActions();
    connectActions();
    return m_menuBar;
}

void QMainMenuHelper::createActions()
{
    m_fileMenu = m_menuBar->addMenu(tr("&File"));
    m_openAction = m_fileMenu->addAction(tr("&Open"));
    m_openAction->setShortcut(QKeySequence::Open);

    m_helpMenu = m_menuBar->addMenu(tr("&Help"));
    m_hotKeysAction = m_helpMenu->addAction(tr("&Hot keys"));
    m_hotKeysAction->setShortcut(QKeySequence(Qt::Key_F1));
    m_contactUsAction = m_helpMenu->addAction(tr("&Contact us"));
}

void QMainMenuHelper::connectActions()
{
    connect(m_openAction, &QAction::triggered, this, &QMainMenuHelper::openFileRequested);
    connect(m_hotKeysAction, &QAction::triggered, this, &QMainMenuHelper::showHotKeys);
    connect(m_contactUsAction, &QAction::triggered, this, &QMainMenuHelper::showContactUs);
}

void QMainMenuHelper::showHotKeys()
{
    QMessageBox::information(
        m_mainWindow,
        tr("Hot Keys"),
        tr(
            "<pre>"
            "Enter      - Zoom In\n"
            "Backspace  - Zoom Out\n"
            "Ctrl+T     - Change Time Domain\n"
            "Ctrl+S     - Save Precise Selection"
            "</pre>"
        )
    );
}

void QMainMenuHelper::showContactUs()
{
    QMessageBox::information(
        m_mainWindow,
        tr("Contact us"),
        tr(
            "<pre>"
            "e-mail : AquaCreatura@gmail.com\n"
            "tg     : @AquaCreatura\n"
            "Phone  : +7 (921) - 6453 - 763\n"
            "If you have any problems or ideas\n - send request in text view"
            "</pre>"
        )
    );
}
