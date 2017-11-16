#ifndef ANDOR_H
#define ANDOR_H

#include "atcore.h"
#include "atutility.h"

class Andor: public QObject {
	Q_OBJECT

private:
	bool m_abort;
	AT_H Hndl;
	bool initialised = FALSE;
	bool connected = FALSE;

	AT_64 imageSizeBytes;
	int i_imageSize;
	unsigned char* gblp_Buffer = NULL;
	unsigned char* pucAlignedBuffer = NULL;

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

	unsigned short* unpackedBuffer;


public slots:
	void checkCamera();
	void acquireSingle();
	void acquireStartStop();

signals:
	void imageAcquired(unsigned short*, AT_64, AT_64);
};

#endif // ANDOR_H
