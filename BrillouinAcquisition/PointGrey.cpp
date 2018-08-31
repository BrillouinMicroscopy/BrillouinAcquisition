#include "stdafx.h"
#include "PointGrey.h"

PointGrey::~PointGrey() {
	disconnectDevice();
}

void PointGrey::connectDevice() {

	m_system = System::GetInstance();
	m_cameraList = m_system->GetCameras();

	int numCameras = m_cameraList.GetSize();

	if (numCameras > 0) {
		// Select camera
		m_camera = m_cameraList.GetByIndex(0);

		INodeMap & nodeMapTLDevice = m_camera->GetTLDeviceNodeMap();

		// Initialize camera
		m_camera->Init();

		m_isConnected = true;
	}

	emit(connectedDevice(m_isConnected));
}

void PointGrey::disconnectDevice() {
	// Deinitialize camera
	m_camera->DeInit();

	// Release reference to the camera
	m_camera = NULL;

	// Clear camera list before releasing system
	m_cameraList.Clear();

	// Release system
	m_system->ReleaseInstance();

	m_isConnected = false;

	emit(connectedDevice(m_isConnected));
}

void PointGrey::configureCamera() {

}
