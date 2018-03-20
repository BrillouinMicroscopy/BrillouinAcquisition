#ifndef ANDOR_H
#define ANDOR_H

#include "atcore.h"
#include "atutility.h"
#include "circularBuffer.h"

class Andor : public QObject {
	Q_OBJECT

private:
	AT_H m_cameraHndl;
	bool m_isInitialised = FALSE;
	bool m_isConnected = FALSE;

	int m_temperatureStatusIndex = 0;
	wchar_t m_temperatureStatus[256];

	// possible parameters
	AT_WC *m_pixelReadoutRates[2] = { L"100 MHz", L"280 MHz" };
	AT_WC *m_pixelEncodings[4] = { L"Mono12", L"Mono12Packed", L"Mono16", L"Mono32" };
	AT_WC *m_cycleModes[2] = { L"Fixed", L"Continuous" };
	AT_WC *m_triggerModes[5] = { L"Internal", L"Software", L"External", L"External Start", L"External Exposure" };
	AT_WC *m_imageBinnings[5] = { L"1x1", L"2x2", L"3x3", L"4x4", L"8x8" };
	AT_WC *m_preAmpGains[3] = { L"12-bit (low noise)", L"12-bit (high well capacity)", L"16-bit (low noise & high well capacity)" };

	AT_64 m_imageHeight = 2048;
	AT_64 m_imageWidth = 2048;
	AT_64 m_imageLeft = 0;
	AT_64 m_imageTop = 0;
	AT_64 m_imageStride;
	AT_WC *m_imageBinning = m_imageBinnings[0];
	double m_exposureTime = 0.5;

	AT_BOOL m_spuriousNoiseFilter = TRUE;

	AT_WC *m_pixelReadoutRate = m_pixelReadoutRates[0];
	AT_WC *m_pixelEncoding = m_pixelEncodings[2];
	AT_WC *m_cycleMode = m_cycleModes[1];
	AT_WC *m_triggerMode = m_triggerModes[1];
	AT_WC *m_preAmpGain = m_preAmpGains[2];

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
