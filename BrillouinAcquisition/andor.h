#ifndef ANDOR_H
#define ANDOR_H

#include "atcore.h"

class Andor: public QObject {
//class Andor {
	Q_OBJECT

private:
	bool m_abort;
	AT_H Hndl;
	bool initialised = FALSE;
	bool connected = FALSE;

	int temperatureStatusIndex = 0;
	wchar_t temperatureStatus[256];

public:
	Andor(QObject *parent = 0);
	~Andor();
	void connect();
	void disconnect();
	bool getConnectionStatus();

	// setters/getters for sensor cooling
	void setSensorCooling(bool cooling);
	bool getSensorCooling();
	const wchar_t getTemperatureStatus();
	double getSensorTemperature();

	// setters/getters for ROI


public slots:
	void checkCamera();
	void getImages();
};

#endif // ANDOR_H
