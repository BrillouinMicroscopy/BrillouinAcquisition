#pragma once
#include "atcore.h"
#include <QThread>
#include <QMutex>

class Andor: public QThread {
//class Andor {

private:
	bool m_abort;
	QMutex mutex;
	AT_H Hndl;
	bool initialised = FALSE;
	bool connected = FALSE;

public:
	Andor(QObject *parent = 0);
	~Andor();

	void checkCamera();

protected:
	void run();
};
