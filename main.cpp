#include "learnwindows.h"
#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	LearnWindows w;
	w.show();
	return a.exec();
}
