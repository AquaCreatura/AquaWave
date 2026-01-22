#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_AquaWave.h"
#include "Arks\ShipBuilder.h"
class AquaWave : public QMainWindow
{
    Q_OBJECT

public:
    AquaWave(QWidget *parent = nullptr, const QString& file_path = QString() );
    ~AquaWave();

private:
    Ui::AquaWaveWindow              ui;
	ShipBuilder						ship_builder_;
	fluctus::ArkSptr				file_src_;
	fluctus::ArkSptr				spectral_viewer;
	fluctus::ArkSptr				scope_analyser;
};
