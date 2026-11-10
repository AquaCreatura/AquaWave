#include "AquaWave.h"
#include <QtWidgets/QApplication>

#include <qdir.h>
#include <Windows.h>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
	QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    const auto cur_icon = QIcon(":/AquaWave/wave.png");
    if(!cur_icon.isNull())
        a.setWindowIcon(cur_icon);
    QString file_path;
	if (argc >= 2) {
		file_path = QString::fromLocal8Bit(argv[1]);
		//Надо просто передавать %P%N
		{// Стучимся в Total Commander за активным путем
			wchar_t b[512] = { 0 };
			GetWindowTextW((HWND)SendMessage(FindWindowA("TTOTAL_CMD", nullptr), 1074, 21, 0), b, 512);
			QString tcDir = QString::fromWCharArray(b).split('>')[0].remove('*').trimmed();
			// 3. Если путь из TC получен — склеиваем его с именем файла, иначе — берем дефолтный абсолютный путь
			file_path = tcDir.isEmpty() ? QFileInfo(file_path).absoluteFilePath() : QDir(tcDir).absoluteFilePath(file_path);
		}
	}
	file_path = "C:/Users/User/Desktop/Waver/QWKWidgets.dll";
	//file_path = "D:/signals/star_signals/iDirect and HUBs 11081.434200MHz 7000.000KHz.pcm";
    AquaWave w(nullptr, file_path);
    w.show();
    return a.exec();
}
