#pragma once
#include "Interfaces/ark_interface.h"
#include <qwindow.h>
class ShipBuilder {

public:
	aqua::ArkSptr BuildNewShip(aqua::ArkType ship_type, QWidget* main = nullptr);
	static bool Bind_SrcSink(aqua::ArkSptr from, aqua::ArkSptr to);
	static QPointer<QWidget> GetWindow(aqua::ArkSptr ship);
protected:
	std::vector<aqua::ArkSptr> fleet;
};