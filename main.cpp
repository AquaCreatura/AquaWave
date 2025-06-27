#include "AquaWave.h"
#include <QtWidgets/QApplication>
#include <thread>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    const auto cur_icon = QIcon(":/AquaWave/wave.png");
    if(!cur_icon.isNull())
        a.setWindowIcon(cur_icon);
    QString file_path;
    if(argc >= 2)
        file_path = QString::fromLocal8Bit(argv[1]);
    AquaWave w(nullptr, file_path);
    w.show();
    return a.exec();
}
