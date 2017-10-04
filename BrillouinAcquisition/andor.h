#pragma once
#include "atcore.h"

class Andor: public QObject {
//class Andor {
	Q_OBJECT

private:
	bool m_abort;
	AT_H Hndl;
	bool initialised = FALSE;
	bool connected = FALSE;

public:
	Andor(QObject *parent = 0);
	~Andor();

public slots:
	void checkCamera();
	void getImages();
};
