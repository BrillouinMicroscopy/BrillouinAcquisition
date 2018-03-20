#ifndef ANDOR_H
#define ANDOR_H

#include "atcore.h"
#include "atutility.h"
#include "circularBuffer.h"

class Andor: public QObject {
	Q_OBJECT

private:
	AT_H m_cameraHndl;
	bool m_isInitialised = FALSE;
	bool m_isConnected = FALSE;

	int m_temperatureStatusIndex = 0;
	wchar_t m_temperatureStatus[256];

	AT_64 m_imageHeight;
	AT_64 m_imageWidth;
	AT_64 m_imageLeft;
	AT_64 m_imageTop;
	AT_64 m_imageStride;

	int m_bufferSize;

	void prepareAcquisition();
	void cleanupAcquisition();

public:
	Andor(QObject *parent = 0);
	~Andor();
	void connect();
	void disconnect();
	bool getConnectionStatus();
	bool m_isAcquiring = FALSE;

	// setters/getters for sensor cooling
	void setSensorCooling(bool cooling);
	bool getSensorCooling();
	const wchar_t getTemperatureStatus();
	double getSensorTemperature();

	// setters/getters for ROI

	// circular buffer for live acquisition
	CircularBuffer<AT_U8>* liveBuffer;

private slots:
	void acquire();

public slots:
	void acquireContinuously();

signals:
	void imageAcquired(unsigned short*, AT_64, AT_64);
	void acquisitionRunning(bool, CircularBuffer<AT_U8>*, AT_64, AT_64);
};

#endif // ANDOR_H
