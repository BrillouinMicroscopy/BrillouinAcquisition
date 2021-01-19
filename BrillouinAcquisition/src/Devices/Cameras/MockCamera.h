#ifndef MOCKCAMERA_H
#define MOCKCAMERA_H

#include "Camera.h"

class MockCamera : public Camera {
	Q_OBJECT

public:
	MockCamera() noexcept {};
	~MockCamera();

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
};

#endif // MOCKCAMERA_H