#ifndef POINTGREY_H
#define POINTGREY_H

#include <gsl/gsl>

#include "FlyCapture2.h"

#include "cameraParameters.h"
#include "previewBuffer.h"

using namespace FlyCapture2;

class PointGrey : public QObject {
	Q_OBJECT

private:
	BusManager m_busManager;
	PGRGuid m_guid;
	Camera m_camera;

	bool m_isConnected{ false };

	CAMERA_OPTIONS m_options;
	CAMERA_SETTINGS m_settings;

	void preparePreview();
	void cleanupAcquisition();

	void acquireImage(unsigned char * buffer);

	bool PollForTriggerReady(Camera * camera);

	bool FireSoftwareTrigger(Camera * camera);

	void readOptions();
	CAMERA_SETTINGS readSettings();

private slots:
	void init() {};
	void getImageForPreview();

public:
	PointGrey() noexcept {};
	~PointGrey();

	bool m_isPreviewRunning = false;

	bool getConnectionStatus();

	void setSettingsPreview();
	void setSettingsMeasurement();
	void stopPreview();

	// preview buffer for live acquisition
	PreviewBuffer<unsigned char>* previewBuffer = new PreviewBuffer<unsigned char>;

public slots:
	void connectDevice();
	void disconnectDevice();

	void startPreview(CAMERA_SETTINGS settings);
	void readImageFromCamera(unsigned char* buffer);

signals:
	void connectedDevice(bool);
	void s_previewRunning(bool);
	void s_previewBufferSettingsChanged();
	void settingsChanged(CAMERA_SETTINGS);
	void optionsChanged(CAMERA_OPTIONS);
};

#endif // POINTGREY_H