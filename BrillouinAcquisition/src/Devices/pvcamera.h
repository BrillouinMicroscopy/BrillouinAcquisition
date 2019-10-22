#ifndef PVCAMERA_H
#define PVCAMERA_H

#include "Camera.h"
#include <typeinfo>

namespace PVCam {
	#include "master.h"
	#include "pvcam.h"

	// Name-Value Pair type - an item in enumeration type
	typedef struct NVP {
		int32 value;
		std::string name;
	} NVP;

	// Name-Value Pair Container type - an enumeration type
	typedef std::vector<NVP> NVPC;

	typedef struct READOUT_OPTION {
		NVP port;
		int16 speedIndex;
		float readoutFrequency;
		int16 bitDepth;
		std::vector<int16> gains;
	} READOUT_OPTION;

}

class PVCamera : public Camera {
	Q_OBJECT

private:
	PVCam::int16 m_camera{ -1 };
	bool m_isInitialised{ false };
	bool m_isCooling{ false };
	QTimer* m_tempTimer = nullptr;
	SensorTemperature m_sensorTemperature;

	void cleanupAcquisition();
	void preparePreview();

	int acquireImage(unsigned char* buffer) override;

	/*
	 * Members and functions inherited from base class
	 */
	void readOptions();
	void readSettings();

	PVCam::rgn_type getCamSettings();

	PVCam::uns16 *m_buffer = nullptr;
	unsigned int m_bufferSize{ 0 };

	 // Vector of camera readout options
	std::vector<PVCam::READOUT_OPTION> m_SpeedTable;

public:
	PVCamera() noexcept {};
	~PVCamera();
	bool initialize();

	// setters/getters for sensor cooling
	bool getSensorCooling();
	const std::string getTemperatureStatus();
	double getSensorTemperature();
	void setCalibrationExposureTime(double);

	bool IsParamAvailable(PVCam::uns32 paramID, const char* paramName);
	bool ReadEnumeration(PVCam::NVPC* nvpc, PVCam::uns32 paramID, const char* paramName);

private slots:
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
	
	void getImageForAcquisition(unsigned char* buffer, bool preview = true) override;
};

#endif // PVCAMERA_H
