#ifndef CAMERA_H
#define CAMERA_H

#include <QtCore>
#include <gsl/gsl>

#include "../Device.h"

#include "cameraParameters.h"
#include "../../lib/buffer_preview.h"

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
	~Camera() {
		if (m_previewBuffer) {
			delete m_previewBuffer;
			m_previewBuffer = nullptr;
		}
	};

	CAMERA_OPTIONS getOptions();
	CAMERA_SETTINGS getSettings();

	bool m_isPreviewRunning{ false };
	bool m_isAcquisitionRunning{ false };

	bool m_stopPreview{ false };
	bool m_stopAcquisition{ false };

	// preview buffer for live acquisition
	PreviewBuffer<std::byte>* m_previewBuffer = new PreviewBuffer<std::byte>;

public slots:
	virtual void startPreview() = 0;
	virtual void stopPreview() = 0;
	virtual void startAcquisition(const CAMERA_SETTINGS&) = 0;
	virtual void stopAcquisition() = 0;
	virtual void getImageForAcquisition(std::byte* buffer, bool preview = true) = 0;

	virtual void setCalibrationExposureTime(double) {};
	virtual void setSensorCooling(bool cooling) {};
	virtual bool getSensorCooling() { return false; };

	void setSettings(CAMERA_SETTINGS);
	void setSetting(CAMERA_SETTING, double);
	void setSetting(CAMERA_SETTING, const std::wstring&);

	int getCameraNumber();
	void setCameraNumber(int cameraNumber);
	int getNumberCameras();

protected:
	virtual int acquireImage(std::byte* buffer) = 0;

	virtual void readOptions() = 0;
	virtual void readSettings() = 0;
	virtual void applySettings(const CAMERA_SETTINGS& settings) = 0;

	int m_cameraNumber{ 0 };
	int m_numberCameras{ 1 };

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
	void s_imageReady();
};

#endif //CAMERA_H