#ifndef POINTGREY_H
#define POINTGREY_H

#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

class PointGrey : public QObject {
	Q_OBJECT

private:

	SystemPtr m_system;
	CameraList m_cameraList;

	CameraPtr m_camera{ NULL };

	bool m_isConnected{ false };

public:
	PointGrey() noexcept {};
	virtual ~PointGrey();

	void connectDevice();
	void disconnectDevice();

	void configureCamera();

signals:
	void connectedDevice(bool);
};

#endif // POINTGREY_H