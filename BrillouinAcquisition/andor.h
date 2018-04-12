#ifndef ANDOR_H
#define ANDOR_H

#include "atcore.h"
#include "atutility.h"
#include "circularBuffer.h"
	
// possible parameters
struct CAMERA_OPTIONS {
	const std::vector<AT_WC*> pixelReadoutRates = { L"100 MHz", L"280 MHz" };
	const std::vector<AT_WC*> pixelEncodings = { L"Mono12", L"Mono12Packed", L"Mono16", L"Mono32" };
	const std::vector<AT_WC*> cycleModes = { L"Fixed", L"Continuous" };
	const std::vector<AT_WC*> triggerModes = { L"Internal", L"Software", L"External", L"External Start", L"External Exposure" };
	const std::vector<AT_WC*> preAmpGains = { L"12-bit (low noise)", L"12-bit (high well capacity)", L"16-bit (low noise & high well capacity)" };
	const std::vector<AT_WC*> imageBinnings = { L"1x1", L"2x2", L"3x3", L"4x4", L"8x8" };
	std::vector<double> exposureTimeLimits = { 0.01, 1 };
	std::vector<AT_64> frameCountLimits = { 1, 100 };
	std::vector<AT_64> ROIWidthLimits = { 1, 2 };
	std::vector<AT_64> ROIHeightLimits = { 1, 2 };
};

struct CAMERA_ROI {
	AT_64 left = 1;
	AT_64 width = 2048;
	AT_64 top = 1;
	AT_64 height = 2048;
	AT_WC binning[256] = L"1x1";
};

struct CAMERA_READOUT {
	AT_WC pixelReadoutRate[256] = L"100 MHz";
	AT_WC pixelEncoding[256] = L"Mono16";
	AT_WC cycleMode[256] = L"Continuous";
	AT_WC triggerMode[256] = L"Software";
	AT_WC preAmpGain[256] = L"16-bit (low noise & high well capacity)";
};

struct CAMERA_SETTINGS {
	double exposureTime = 0.5;				// [s]	exposure time
	AT_64 frameCount = 2;					// [1]	number of images to acquire in one sequence
	AT_BOOL spuriousNoiseFilter = true;		//		turn on spurious noise filter
	CAMERA_ROI roi;							//		region of interest
	CAMERA_READOUT readout;					//		readout settings
};

class Andor : public QObject {
	Q_OBJECT

private:
	AT_H m_cameraHndl;
	bool m_isInitialised = false;
	bool m_isConnected = false;

	int m_temperatureStatusIndex = 0;
	AT_WC m_temperatureStatus[256];

	AT_64 m_imageStride;

	CAMERA_OPTIONS m_options;
	CAMERA_SETTINGS m_settings;

	int m_bufferSize;

	void readOptions();
	void setDefaultSettings();
	void setSettings();
	void readSettings();
	void prepareAcquisition();
	void getEnumString(AT_WC* feature, AT_WC* string);

public:
	Andor(QObject *parent = 0);
	~Andor();
	void connect();
	void disconnect();
	bool getConnectionStatus();
	bool m_isAcquiring = false;

	// setters/getters for sensor cooling
	void setSensorCooling(bool cooling);
	bool getSensorCooling();
	const wchar_t getTemperatureStatus();
	double getSensorTemperature();
	void cleanupAcquisition();

	// setters/getters for ROI

	// circular buffer for live acquisition
	CircularBuffer<AT_U8>* liveBuffer;

private slots:
	void acquire();

public slots:
	void acquireContinuously();
	void prepareMeasurement(CAMERA_SETTINGS settings);
	void acquireImage(AT_U8* buffer);

signals:
	void imageAcquired(unsigned short*, AT_64, AT_64);
	void acquisitionRunning(bool, CircularBuffer<AT_U8>*, AT_64, AT_64);
	void settingsChanged(CAMERA_SETTINGS);
	void optionsChanged(CAMERA_OPTIONS);
};

#endif // ANDOR_H
