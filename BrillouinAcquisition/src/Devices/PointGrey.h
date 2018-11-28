#ifndef POINTGREY_H
#define POINTGREY_H

#include "Camera.h"

#include "FlyCapture2.h"

//using namespace FlyCapture2;

class PointGrey : public Camera {
	Q_OBJECT

private:
	/*
	 * Members and functions specific to PointGrey class
	 */
	FlyCapture2::Camera m_camera;
	FlyCapture2::BusManager m_busManager;
	FlyCapture2::PGRGuid m_guid;

	bool PollForTriggerReady(FlyCapture2::Camera* camera);
	bool FireSoftwareTrigger(FlyCapture2::Camera* camera);

	void preparePreview();

	void acquireImage(unsigned char* buffer) override;

	/*
	 * Members and functions inherited from base class
	 */
	void readOptions();
	void readSettings();

private slots:
	void getImageForPreview() override;

public:
	PointGrey() noexcept {};
	~PointGrey();

public slots:
	void init() {};
	void connectDevice();
	void disconnectDevice();

	void setSettings(CAMERA_SETTINGS);

	void startPreview();
	void stopPreview();
	void startAcquisition(CAMERA_SETTINGS);
	void stopAcquisition();

	void getImageForAcquisition(unsigned char* buffer) override;
};

#endif // POINTGREY_H