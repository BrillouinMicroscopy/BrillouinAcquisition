#include "stdafx.h"
#include "PointGrey.h"

PointGrey::~PointGrey() {
	disconnectDevice();
}

void PointGrey::connectDevice() {
	if (!m_isConnected) {
		m_system = System::GetInstance();
		m_cameraList = m_system->GetCameras();

		int numCameras = m_cameraList.GetSize();

		if (numCameras > 0) {
			// Select camera
			m_camera = m_cameraList.GetByIndex(0);

			// Retrieve TL device nodemap
			m_nodeMapTLDevice = &m_camera->GetTLDeviceNodeMap();

			// Initialize camera
			m_camera->Init();

			// Retrieve GenICam nodemap
			m_nodeMap = &m_camera->GetNodeMap();

			m_isConnected = true;
		}
	}

	emit(connectedDevice(m_isConnected));
}

void PointGrey::disconnectDevice() {
	if (m_isConnected) {
		// Deinitialize camera
		m_camera->DeInit();

		// Release reference to the camera
		m_camera = NULL;

		// Clear camera list before releasing system
		m_cameraList.Clear();

		// Release system
		m_system->ReleaseInstance();

		m_isConnected = false;
	}

	emit(connectedDevice(m_isConnected));
}

bool PointGrey::getConnectionStatus() {
	return m_isConnected;
}

void PointGrey::setSettings() {
	/*
	 * Set acquisition to continuous
	 */
	// Retrieve enumeration node from nodemap
	CEnumerationPtr ptrAcquisitionMode = m_nodeMap->GetNode("AcquisitionMode");
	// Retrieve entry node from enumeration node
	CEnumEntryPtr ptrAcquisitionModeContinuous = ptrAcquisitionMode->GetEntryByName("Continuous");
	// Retrieve integer value from entry node and set integer value from entry node as new value of enumeration node
	ptrAcquisitionMode->SetIntValue(ptrAcquisitionModeContinuous->GetValue());

	/*
	* Set pixel format to Mono8
	*/
	CEnumerationPtr ptrPixelformat = m_nodeMap->GetNode("PixelFormat");
	CEnumEntryPtr ptrPixelformatRaw = ptrPixelformat->GetEntryByName("Mono8");
	ptrPixelformat->SetIntValue(ptrPixelformatRaw->GetValue());

	/*
	* Set region of interest
	*/
	// Offset x to minimum
	CIntegerPtr ptrOffsetX = m_nodeMap->GetNode("OffsetX");
	if (IsAvailable(ptrOffsetX) && IsWritable(ptrOffsetX)) {
		int64_t offsetX = ptrOffsetX->GetMin();
		m_settings.roi.left = offsetX;
		ptrOffsetX->SetValue(offsetX);
	}
	// Offset y to minimum
	CIntegerPtr ptrOffsetY = m_nodeMap->GetNode("OffsetY");
	if (IsAvailable(ptrOffsetY) && IsWritable(ptrOffsetY)) {
		int64_t offsetY = ptrOffsetY->GetMin();
		m_settings.roi.top = offsetY;
		ptrOffsetY->SetValue(offsetY);
	}
	// Width to maximum
	CIntegerPtr ptrWidth = m_nodeMap->GetNode("Width");
	if (IsAvailable(ptrWidth) && IsWritable(ptrWidth)) {
		int64_t widthToSet = ptrWidth->GetMax();
		m_settings.roi.width = widthToSet;
		ptrWidth->SetValue(widthToSet);
	}
	// Height to maximum
	CIntegerPtr ptrHeight = m_nodeMap->GetNode("Height");
	if (IsAvailable(ptrHeight) && IsWritable(ptrHeight)) {
		int64_t heightToSet = ptrHeight->GetMax();
		m_settings.roi.height = heightToSet;
		ptrHeight->SetValue(heightToSet);
	}
}

void PointGrey::startPreview(CAMERA_SETTINGS settings) {
	m_isPreviewRunning = true;
	m_settings = settings;
	preparePreview();
	getImageForPreview();
}

void PointGrey::preparePreview() {
	// always use full camera image for live preview
	m_settings.roi.width = 1280;
	m_settings.roi.left = 1;
	m_settings.roi.height = 1024;
	m_settings.roi.top = 1;

	setSettings();

	int pixelNumber = m_settings.roi.width * m_settings.roi.height;
	BUFFER_SETTINGS bufferSettings = { 4, pixelNumber, m_settings.roi };
	previewBuffer->initializeBuffer(bufferSettings);
	emit(s_previewBufferSettingsChanged());

	m_camera->BeginAcquisition();

	emit(s_previewRunning(true));
}

void PointGrey::stopPreview() {
	m_isPreviewRunning = false;
	cleanupAcquisition();
	emit(s_previewRunning(false));
}

void PointGrey::cleanupAcquisition() {
	m_camera->EndAcquisition();
}

void PointGrey::getImageForPreview() {
	if (m_isPreviewRunning) {

		previewBuffer->m_buffer->m_freeBuffers->acquire();
		acquireImage(previewBuffer->m_buffer->getWriteBuffer());
		previewBuffer->m_buffer->m_usedBuffers->release();

		QMetaObject::invokeMethod(this, "getImageForPreview", Qt::QueuedConnection);
	} else {
		stopPreview();
	}
}

void PointGrey::acquireImage(unsigned char* buffer) {

	ImagePtr pResultImage = m_camera->GetNextImage();

	if (!pResultImage->IsIncomplete()) {
		ImagePtr convertedImage = pResultImage->Convert(PixelFormat_Mono8);

		unsigned char* data = static_cast<unsigned char*>(convertedImage->GetData());

		memcpy(buffer, data, m_settings.roi.width*m_settings.roi.height);
	}

	pResultImage->Release();
};