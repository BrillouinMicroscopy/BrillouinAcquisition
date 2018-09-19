#ifndef POINTGREY_H
#define POINTGREY_H

#include "Camera.h"

#include "FlyCapture2.h"

//using namespace FlyCapture2;

class PointGrey : public Camera {
	Q_OBJECT

private:
	FlyCapture2::BusManager m_busManager;
	FlyCapture2::PGRGuid m_guid;

	bool PollForTriggerReady(FlyCapture2::Camera * camera);
	bool FireSoftwareTrigger(FlyCapture2::Camera * camera);

	void preparePreview();
	void cleanupAcquisition();

	void acquireImage(unsigned char * buffer);

	void readOptions();
	CAMERA_SETTINGS readSettings();

private slots:
	void getImageForPreview();

public:
	PointGrey() noexcept {};
	~PointGrey();
	FlyCapture2::Camera m_camera;

	void setSettingsPreview();
	void setSettingsMeasurement();
	void stopPreview();

	// preview buffer for live acquisition
	PreviewBuffer<unsigned char>* previewBuffer = new PreviewBuffer<unsigned char>;

public slots:
	void init() {};
	bool connectDevice();
	bool disconnectDevice();

	void startPreview(CAMERA_SETTINGS settings);
	void readImageFromCamera(unsigned char* buffer);

signals:
	void s_previewRunning(bool);
};

#endif // POINTGREY_H