#ifndef ANDOR_H
#define ANDOR_H

#include "atcore.h"
#include "atutility.h"
#include "cameraParameters.h"
#include "previewBuffer.h"

typedef enum enAndorTemperatureStatus{
	COOLER_OFF,
	FAULT,
	COOLING,
	DRIFT,
	NOT_STABILISED,
	STABILISED
} ANDOR_TEMPERATURE_STATUS;

typedef struct {
	double temperature = 0;
	ANDOR_TEMPERATURE_STATUS status = COOLER_OFF;
} SensorTemperature;

class Andor : public QObject {
	Q_OBJECT

private:
	AT_H m_cameraHndl = -1;
	bool m_isInitialised = false;
	bool m_isConnected = false;
	bool m_isCooling = false;

	int m_temperatureStatusIndex = 0;
	AT_WC temperatureStatus[256] = {};
	std::string m_temperatureStatus;
	QTimer *m_tempTimer = nullptr;
	SensorTemperature m_sensorTemperature;
	int m_test = 0;

	AT_64 m_imageStride = 0;

	CAMERA_OPTIONS m_options;
	CAMERA_SETTINGS m_settings;

	int m_bufferSize = -1;

	void readOptions();
	void setDefaultSettings();
	void setSettings();
	CAMERA_SETTINGS readSettings();
	void preparePreview();
	void getEnumString(AT_WC* feature, AT_WC* string);

public:
	Andor(QObject *parent = nullptr) noexcept;
	~Andor();
	bool initialize();
	bool getConnectionStatus();
	bool m_isPreviewRunning = false;

	// setters/getters for sensor cooling
	bool getSensorCooling();
	const std::string getTemperatureStatus();
	double getSensorTemperature();
	void cleanupAcquisition();
	void setCalibrationExposureTime(double);

	// setters/getters for ROI

	// preview buffer for live acquisition
	PreviewBuffer<AT_U8>* previewBuffer = new PreviewBuffer<AT_U8>;

private slots:
	void init();
	void acquireImage(AT_U8* buffer);
	void getImageForPreview();
	void checkSensorTemperature();

public slots:
	void connectDevice();
	void disconnectDevice();
	void startPreview(CAMERA_SETTINGS settings);
	void stopPreview();
	void stopMeasurement();
	CAMERA_SETTINGS prepareMeasurement(CAMERA_SETTINGS settings);
	void getImageForMeasurement(AT_U8* buffer);
	// setters/getters for sensor cooling
	void setSensorCooling(bool cooling);

signals:
	void cameraConnected(bool);
	void cameraCoolingChanged(bool);
	void noCameraFound();
	void s_previewRunning(bool);
	void s_measurementRunning(bool);
	void settingsChanged(CAMERA_SETTINGS);
	void optionsChanged(CAMERA_OPTIONS);
	void s_previewBufferSettingsChanged();
	void s_sensorTemperatureChanged(SensorTemperature);
};

#endif // ANDOR_H
