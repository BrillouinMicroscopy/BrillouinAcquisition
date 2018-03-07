#ifndef ANDOR_H
#define ANDOR_H

#include "atcore.h"
#include "atutility.h"

class Andor: public QObject {
	Q_OBJECT

private:
	AT_H m_cameraHndl;
	bool m_isInitialised = FALSE;
	bool m_isConnected = FALSE;
	bool m_isAcquiring = FALSE;

	int m_temperatureStatusIndex = 0;
	wchar_t m_temperatureStatus[256];

	AT_64 m_imageHeight;
	AT_64 m_imageWidth;
	AT_64 m_imageStride;

	AT_64 imageSizeBytes;
	int i_imageSize;
	unsigned char* gblp_Buffer = NULL;
	unsigned char* pucAlignedBuffer = NULL;

	int BufferSize;
	unsigned char* UserBuffer;
	unsigned char* Buffer;

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
	void acquire();
	void acquireContinuously();

	void acquireSingleTest(int index, std::string test);
	void acquireSingle();
	void acquireStartStop();

signals:
	void imageAcquired(unsigned short*, AT_64, AT_64);
};

#endif // ANDOR_H
