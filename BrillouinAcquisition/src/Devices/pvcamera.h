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

	static void previewCallback(PVCam::FRAME_INFO* pFrameInfo, void* context);
	static void acquisitionCallback(PVCam::FRAME_INFO* pFrameInfo, void* context);
	PVCam::uns16* m_acquisitionBuffer = nullptr;
	std::mutex g_EofMutex;
	std::condition_variable g_EofCond;
	// New frame flag that helps with spurious wakeups.
	// For really fast acquisitions is better to replace this flag with some queue
	// storing new frame address and frame info delayed processing.
	// Otherwise some frames might be lost.
	bool g_EofFlag{ false };
	void getImageForPreview() override;

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