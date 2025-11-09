#include <QApplication>
#include "main-window.h"

int main(int argc, char *argv[]) {
	QApplication a(argc, argv);
#ifdef Q_OS_WIN32
	a.setStyle("windowsvista");
#endif
	MainWindow w;
	w.show();

	return a.exec();
}
