#include "AquaWave.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    const auto cur_icon = QIcon(":/AquaWave/wave.png");
    if(!cur_icon.isNull())
        a.setWindowIcon(cur_icon);
    AquaWave w;
    w.show();
    return a.exec();
}
