#ifndef ANDOR_H
#define ANDOR_H

#include "atcore.h"
#include "atutility.h"
#include "cameraParameters.h"
#include "previewBuffer.h"

class Andor : public QObject {
	Q_OBJECT

private:
	AT_H m_cameraHndl = -1;
	bool m_isInitialised = false;
	bool m_isConnected = false;
	bool m_isCooling = false;

	int m_temperatureStatusIndex = 0;
	AT_WC m_temperatureStatus[256] = {};

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
	const wchar_t getTemperatureStatus();
	double getSensorTemperature();
	void cleanupAcquisition();
	void setCalibrationExposureTime(double);

	// setters/getters for ROI

	// preview buffer for live acquisition
	PreviewBuffer<AT_U8>* previewBuffer = new PreviewBuffer<AT_U8>;

private slots:
	void acquireImage(AT_U8* buffer);
	void getImageForPreview();

public slots:
	void connectDevice();
	void disconnectDevice();
	void startPreview(CAMERA_SETTINGS settings);
	void stopPreview();
	CAMERA_SETTINGS prepareMeasurement(CAMERA_SETTINGS settings);
	void getImageForMeasurement(AT_U8* buffer);
	// setters/getters for sensor cooling
	void setSensorCooling(bool cooling);

signals:
	void cameraConnected(bool);
	void cameraCoolingChanged(bool);
	void noCameraFound();
	void s_previewRunning(bool);
	void settingsChanged(CAMERA_SETTINGS);
	void optionsChanged(CAMERA_OPTIONS);
	void s_previewBufferSettingsChanged();
};

#endif // ANDOR_H
