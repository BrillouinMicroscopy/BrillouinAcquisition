#ifndef PVCAMERA_H
#define PVCAMERA_H

#include "Camera.h"
#include <string>
#include <codecvt>
#include <locale>
#include <typeinfo>

namespace PVCam {
#include "master.h"
#include "pvcam.h"
}

class PVCamera : public Camera {
	Q_OBJECT

public:
	PVCamera() noexcept {};
	~PVCamera();

public slots:
	void init() override;
	void connectDevice() override;
	void disconnectDevice() override;

	void startPreview() override;
	void stopPreview() override;
	void startAcquisition(CAMERA_SETTINGS) override;
	void stopAcquisition() override;
	void getImageForAcquisition(std::byte* buffer, bool preview = true) override;

	void setCalibrationExposureTime(double) override;
	void setSensorCooling(bool cooling) override;
	bool getSensorCooling() override;

private:
	int acquireImage(std::byte* buffer) override;

	void readOptions() override;
	void readSettings() override;
	void applySettings(CAMERA_SETTINGS settings) override;

	bool initialize();

	const std::string getTemperatureStatus();
	double getSensorTemperature();

	static void previewCallback(PVCam::FRAME_INFO* pFrameInfo, void* context);
	static void acquisitionCallback(PVCam::FRAME_INFO* pFrameInfo, void* context);

	void startTempTimer();
	void stopTempTimer();

	// Name-Value Pair type - an item in enumeration type
	typedef struct NVP {
		PVCam::int32 value{ 0 };
		std::string name;
	} NVP;

	// Name-Value Pair Container type - an enumeration type
	typedef std::vector<NVP> NVPC;

	typedef struct READOUT_OPTION {
		NVP port;
		PVCam::int16 speedIndex{ 0 };
		float readoutFrequency{ 0 };
		PVCam::int16 bitDepth{ 1 };
		std::vector<PVCam::int16> gains;
		std::wstring label;
	} READOUT_OPTION;

	bool IsParamAvailable(PVCam::uns32 paramID, const char* paramName);
	bool ReadEnumeration(NVPC* nvpc, PVCam::uns32 paramID, const char* paramName);

	PVCam::rgn_type getCamSettings();

	PVCam::int16 m_camera{ -1 };
	bool m_isInitialised{ false };
	bool m_isCooling{ false };
	QTimer* m_tempTimer{ nullptr };
	SensorTemperature m_sensorTemperature;

	PVCam::uns16* m_acquisitionBuffer{ nullptr };
	std::mutex g_EofMutex;
	std::condition_variable g_EofCond;
	// New frame flag that helps with spurious wakeups.
	// For really fast acquisitions is better to replace this flag with some queue
	// storing new frame address and frame info delayed processing.
	// Otherwise some frames might be lost.
	bool g_EofFlag{ false };

	PVCam::uns16* m_buffer{ nullptr };
	unsigned int m_bufferSiz1e{ 0 };

	// Vector of camera readout options
	std::vector<READOUT_OPTION> m_SpeedTable;

private slots:
	void getImageForPreview() override;

	void preparePreview();
	void cleanupPreview();

	void prepareAcquisition(CAMERA_SETTINGS settings);
	void cleanupAcquisition();

	void checkSensorTemperature();
};

#endif // PVCAMERA_H
