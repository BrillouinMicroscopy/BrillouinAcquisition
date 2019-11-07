#ifndef POINTGREY_H
#define POINTGREY_H

#include "Camera.h"

#include "FlyCapture2.h"

//using namespace FlyCapture2;

class PointGrey : public Camera {
	Q_OBJECT

public:
	PointGrey() noexcept {};
	~PointGrey();

public slots:
	void init() override {};
	void connectDevice() override;
	void disconnectDevice() override;

	void setSettings(CAMERA_SETTINGS) override;
	void startPreview() override;
	void stopPreview() override;
	void startAcquisition(CAMERA_SETTINGS) override;
	void stopAcquisition() override;
	void getImageForAcquisition(unsigned char* buffer, bool preview = true) override;

private:
	int acquireImage(unsigned char* buffer) override;

	void readOptions() override;
	void readSettings() override;

	void preparePreview();

	bool PollForTriggerReady();
	bool FireSoftwareTrigger();

	FlyCapture2::Camera m_camera;
	FlyCapture2::BusManager m_busManager;
	FlyCapture2::PGRGuid m_guid;
};

#endif // POINTGREY_H