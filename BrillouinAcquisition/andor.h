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

	AT_64 imageSizeBytes;
	int i_imageSize;
	unsigned char* gblp_Buffer = NULL;
	unsigned char* pucAlignedBuffer = NULL;

	int BufferSize;
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
	CircularBuffer<AT_U8>* liveBuffer;

	// setters/getters for ROI

public slots:
	void acquire();
	void acquireContinuously();

	void acquireSingleTest(int index, std::string test);
	void acquireSingle();
	void acquireStartStop();

signals:
	void imageAcquired(unsigned short*, AT_64, AT_64);
	void acquisitionRunning(bool, CircularBuffer<AT_U8>*, AT_64, AT_64);
};

#endif // ANDOR_H
