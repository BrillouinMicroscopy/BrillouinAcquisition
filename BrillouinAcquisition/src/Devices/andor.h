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
	/*
	 * Members and functions specific to Andor class
	 */
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

	void cleanupAcquisition();
	void getEnumString(AT_WC* feature, AT_WC* string);
	void preparePreview();

	void acquireImage(AT_U8* buffer);

	/*
	 * Members and functions inherited from base class
	 */
	void readOptions();
	void readSettings();

public:
	Andor() noexcept {};
	~Andor();
	bool initialize();

	/*
	 * Members and functions specific to Andor class
	 */
	// setters/getters for sensor cooling
	bool getSensorCooling();
	const std::string getTemperatureStatus();
	double getSensorTemperature();
	void setCalibrationExposureTime(double);

	// preview buffer for live acquisition
	PreviewBuffer<AT_U8>* m_previewBuffer = new PreviewBuffer<AT_U8>;

private slots:
	void getImageForPreview();
	void checkSensorTemperature();

public slots:
	/*
	* Members and functions specific to Andor class
	*/
	// setters/getters for sensor cooling
	void setSensorCooling(bool cooling);

	/*
	* Members and functions inherited from base class
	*/
	void init();
	void connectDevice();
	void disconnectDevice();

	void setSettings(CAMERA_SETTINGS);

	void startPreview();
	void stopPreview();
	void startAcquisition(CAMERA_SETTINGS);
	void stopAcquisition();

	/*
	 * Unify me
	 */
	void getImageForAcquisition(AT_U8* buffer);

signals:
	void cameraCoolingChanged(bool);
	void noCameraFound();
	void s_sensorTemperatureChanged(SensorTemperature);
};

#endif // ANDOR_H
