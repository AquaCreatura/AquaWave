#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_AquaWave.h"
#include "Arks/ShipBuilder.h"

class QWindowKitHelper;
class QMainMenuHelper;

class AquaWave : public QMainWindow
{
    Q_OBJECT

public:
    explicit AquaWave(QWidget *parent = nullptr, const QString& file_path = QString());
    ~AquaWave() override;

private:
    void setupWindowAgent();

    struct Page {
        QWidget*   widget;
        std::function<void(std::shared_ptr<fluctus::DoveParrent>)> postDove;
        QString    title;
        QString    iconPath;
    };

    std::vector<Page> pages_;

    Ui::AquaWaveWindow              ui;
    ShipBuilder                     ship_builder_;
    fluctus::ArkSptr                file_src_;
    fluctus::ArkSptr                spectral_viewer_;
    fluctus::ArkSptr                selection_writer_;
    fluctus::ArkSptr                demodulator_;
    QWindowKitHelper*               m_windowKitHelper = nullptr;
    QMainMenuHelper*                m_mainMenuHelper = nullptr;
};
