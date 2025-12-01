#include "Interfaces\ark_interface.h"
#include <qwindow.h>
class ShipBuilder {

public:
	fluctus::ArkSptr BuildNewShip(fluctus::ArkType ship_type);
	static bool Bind_SrcSink(fluctus::ArkSptr from, fluctus::ArkSptr to);
	static QPointer<QWidget> GetWindow(fluctus::ArkSptr ship);
protected:
	std::vector<fluctus::ArkSptr> fleet;
};