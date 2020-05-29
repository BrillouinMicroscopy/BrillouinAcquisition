#ifndef CAMERA_H
#define CAMERA_H

#include <QtCore>
#include <gsl/gsl>

#include "Device.h"

#include "cameraParameters.h"
#include "..\previewBuffer.h"

typedef enum class enCameraTemperatureStatus {
	COOLER_OFF,
	FAULT,
	COOLING,
	DRIFT,
	NOT_STABILISED,
	STABILISED
} CAMERA_TEMPERATURE_STATUS;

typedef struct SensorTemperature {
	double temperature{ 0 };
	double setpoint{ 0 };
	double minSetpoint{ 0 };
	double maxSetpoint{ 0 };
	CAMERA_TEMPERATURE_STATUS status = enCameraTemperatureStatus::COOLER_OFF;
} SensorTemperature;

class Camera : public Device {
	Q_OBJECT

public:
	Camera() {};
	~Camera() {};

	CAMERA_OPTIONS getOptions();
	CAMERA_SETTINGS getSettings();

	bool m_isPreviewRunning{ false };
	bool m_isAcquisitionRunning{ false };

	bool m_stopPreview{ false };
	bool m_stopAcquisition{ false };

	// preview buffer for live acquisition
	PreviewBuffer<unsigned char>* m_previewBuffer = new PreviewBuffer<unsigned char>;

public slots:
	virtual void setSettings(CAMERA_SETTINGS) = 0;
	virtual void startPreview() = 0;
	virtual void stopPreview() = 0;
	virtual void startAcquisition(CAMERA_SETTINGS) = 0;
	virtual void stopAcquisition() = 0;
	virtual void getImageForAcquisition(unsigned char* buffer, bool preview = true) = 0;

	virtual void setCalibrationExposureTime(double) {};
	virtual void setSensorCooling(bool cooling) {};
	virtual bool getSensorCooling() { return false; };

	void setSetting(CAMERA_SETTING, double);

protected:
	virtual int acquireImage(unsigned char* buffer) = 0;

	virtual void readOptions() = 0;
	virtual void readSettings() = 0;

	CAMERA_OPTIONS m_options;
	CAMERA_SETTINGS m_settings;

	std::mutex m_mutex;

	bool m_wasPreviewRunning{ false };		// Was the preview running before we started an acquisition?

protected slots:
	virtual void getImageForPreview();

signals:
	void settingsChanged(CAMERA_SETTINGS);
	void optionsChanged(CAMERA_OPTIONS);
	void s_previewBufferSettingsChanged();
	void s_previewRunning(bool);
	void s_acquisitionRunning(bool);
	void cameraCoolingChanged(bool);
	void noCameraFound();
	void s_sensorTemperatureChanged(SensorTemperature);
};

#endif //CAMERA_H