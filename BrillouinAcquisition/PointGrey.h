#ifndef POINTGREY_H
#define POINTGREY_H

#include <gsl/gsl>

#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"

#include "cameraParameters.h"
#include "previewBuffer.h"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

class PointGrey : public QObject {
	Q_OBJECT

private:

	SystemPtr m_system;
	CameraList m_cameraList;

	CameraPtr m_camera{ NULL };
	INodeMap *m_nodeMapTLDevice = nullptr;
	INodeMap *m_nodeMap;

	bool m_isConnected{ false };

	CAMERA_OPTIONS m_options;
	CAMERA_SETTINGS m_settings;

	void preparePreview();
	void cleanupAcquisition();

	void acquireImage(unsigned char * buffer);

private slots:
	void init() {};
	void getImageForPreview();

public:
	PointGrey() noexcept {};
	~PointGrey();

	bool m_isPreviewRunning = false;

	bool getConnectionStatus();

	void setSettings();
	void stopPreview();

	// preview buffer for live acquisition
	PreviewBuffer<unsigned char>* previewBuffer = new PreviewBuffer<unsigned char>;

public slots:
	void connectDevice();
	void disconnectDevice();

	void startPreview(CAMERA_SETTINGS settings);

signals:
	void connectedDevice(bool);
	void s_previewRunning(bool);
	void s_previewBufferSettingsChanged();
};

#endif // POINTGREY_H