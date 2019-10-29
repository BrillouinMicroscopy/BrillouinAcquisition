#ifndef UEYECAM_H
#define UEYECAM_H

#include "Camera.h"
namespace uEye {
	#include "uEye.h"
}

class uEyeCam : public Camera {
	Q_OBJECT

private:
	/*
	 * Members and functions specific to uEye class
	 */
	uEye::HCAM m_camera = (uEye::HCAM)0;

	void preparePreview();

	int acquireImage(unsigned char* buffer);

	char *m_imageBuffer = nullptr;
	int m_imageBufferId{ 0 };

	/*
	 * Members and functions inherited from base class
	 */
	void readOptions();
	void readSettings();

public:
	uEyeCam() noexcept {};
	~uEyeCam();

public slots:
	void init() {};
	void connectDevice();
	void disconnectDevice();

	void setSettings(CAMERA_SETTINGS);

	void startPreview();
	void stopPreview();
	void startAcquisition(CAMERA_SETTINGS);
	void stopAcquisition();

	void getImageForAcquisition(unsigned char* buffer, bool preview = true) override;
};

#endif // UEYECAM_H