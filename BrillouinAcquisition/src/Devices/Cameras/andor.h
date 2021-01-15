#ifndef ANDOR_H
#define ANDOR_H

#include "Camera.h"
#include <typeinfo>

#include "atcore.h"
#include "atutility.h"

class Andor : public Camera {
	Q_OBJECT

public:
	Andor() noexcept {};
	~Andor();

public slots:
	void init() override;
	void connectDevice() override;
	void disconnectDevice() override;

	void setSettings(CAMERA_SETTINGS) override;
	void startPreview() override;
	void stopPreview() override;
	void startAcquisition(CAMERA_SETTINGS) override;
	void stopAcquisition() override;
	void getImageForAcquisition(unsigned char* buffer, bool preview = true) override;

	void setCalibrationExposureTime(double) override;
	void setSensorCooling(bool cooling) override;
	bool getSensorCooling() override;

private:
	int acquireImage(unsigned char* buffer) override;

	void readOptions() override;
	void readSettings() override;

	bool initialize();

	void preparePreview();
	void cleanupAcquisition();

	const std::string getTemperatureStatus();
	double getSensorTemperature();

	void getEnumString(AT_WC* feature, std::wstring* string);

	AT_H m_camera{ -1 };
	bool m_isInitialised{ false };
	bool m_isCooling{ false };

	int m_temperatureStatusIndex{ 0 };
	std::string m_temperatureStatus{ "" };
	QTimer* m_tempTimer{ nullptr };
	SensorTemperature m_sensorTemperature;
	AT_64 m_imageStride{ 0 };

private slots:
	void checkSensorTemperature();
};

#endif // ANDOR_H
