#pragma once

#include <QObject>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QMainWindow>

class QMainMenuHelper : public QObject
{
    Q_OBJECT

public:
    explicit QMainMenuHelper(QMainWindow* mainWindow);
    ~QMainMenuHelper() override = default;

    QMenuBar* createMenuBar(QWidget* parent = nullptr);
    QMenuBar* menuBar() const { return m_menuBar; }

signals:
    void openFileRequested();

private:
    QMainWindow* m_mainWindow = nullptr;
    QMenuBar* m_menuBar = nullptr;
    QMenu* m_fileMenu = nullptr;
    QMenu* m_helpMenu = nullptr;
    QAction* m_openAction = nullptr;
    QAction* m_hotKeysAction = nullptr;
    QAction* m_contactUsAction = nullptr;

    void createActions();
    void connectActions();

    void showHotKeys();
    void showContactUs();
};
