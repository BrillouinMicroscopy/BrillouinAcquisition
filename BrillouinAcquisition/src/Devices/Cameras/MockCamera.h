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

	void startPreview() override;
	void stopPreview() override;
	void startAcquisition(CAMERA_SETTINGS) override;
	void stopAcquisition() override;
	void getImageForAcquisition(std::byte* buffer, bool preview = true) override;

	void setCalibrationExposureTime(double exposureTime);

private:
	int acquireImage(std::byte* buffer) override;

	template <typename T>
	int acquireImage(std::byte* buffer);

	void readOptions() override;
	void readSettings() override;
	void applySettings(CAMERA_SETTINGS settings) override;

	void preparePreview();
	void preparePreviewBuffer();
};

#endif // MOCKCAMERA_H