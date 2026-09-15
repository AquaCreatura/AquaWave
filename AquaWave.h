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
	struct Page {
		QWidget*   widget;
		// ќбобщЄнный вызов PostDove Ч не зависим от конкретного типа контроллера
		std::function<void(std::shared_ptr<fluctus::DoveParrent>)> postDove;
		QString    title;
		QString    iconPath;
	};

	std::vector<Page> pages_;

    Ui::AquaWaveWindow              ui;
	ShipBuilder						ship_builder_;
	fluctus::ArkSptr				file_src_;
	fluctus::ArkSptr				spectral_viewer_;
	fluctus::ArkSptr				scope_analyser_;
	fluctus::ArkSptr				selection_writer_;
};
