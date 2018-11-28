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

	void acquireImage(unsigned char* buffer);

	/*
	 * Members and functions inherited from base class
	 */
	void readOptions();
	void readSettings();

private slots:
	void getImageForPreview();

public:
	uEyeCam() noexcept {};
	~uEyeCam();

	// preview buffer for live acquisition
	PreviewBuffer<unsigned char>* m_previewBuffer = new PreviewBuffer<unsigned char>;

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

#endif // UEYECAM_H