#ifndef ANDOR_H
#define ANDOR_H

#include "Camera.h"

#include "atcore.h"
#include "atutility.h"

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

class Andor : public Camera {
	Q_OBJECT

private:
	AT_H m_camera{ -1 };
	bool m_isInitialised{ false };
	bool m_isCooling{ false };

	int m_temperatureStatusIndex = 0;
	AT_WC temperatureStatus[256] = {};
	std::string m_temperatureStatus;
	QTimer *m_tempTimer = nullptr;
	SensorTemperature m_sensorTemperature;

	AT_64 m_imageStride = 0;

	int m_bufferSize = -1;

	void readOptions();
	void setDefaultSettings();
	void setSettings();
	CAMERA_SETTINGS readSettings();
	void preparePreview();
	void getEnumString(AT_WC* feature, AT_WC* string);

public:
	Andor() noexcept {};
	~Andor();
	bool initialize();

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
	void acquireImage(AT_U8* buffer);
	void getImageForPreview();
	void checkSensorTemperature();

public slots:
	void init();
	bool connectDevice();
	bool disconnectDevice();
	void startPreview(CAMERA_SETTINGS settings);
	void stopPreview();
	void stopMeasurement();
	CAMERA_SETTINGS prepareMeasurement(CAMERA_SETTINGS settings);
	void getImageForMeasurement(AT_U8* buffer);
	// setters/getters for sensor cooling
	void setSensorCooling(bool cooling);

signals:
	void cameraCoolingChanged(bool);
	void noCameraFound();
	void s_previewRunning(bool);
	void s_measurementRunning(bool);
	void s_sensorTemperatureChanged(SensorTemperature);
};

#endif // ANDOR_H
