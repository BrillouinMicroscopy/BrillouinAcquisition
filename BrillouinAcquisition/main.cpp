#include "stdafx.h"
#include "BrillouinAcquisition.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[]) {
	QApplication a(argc, argv);
	BrillouinAcquisition w;
	w.show();
	return a.exec();
}
