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
	m_camera->AcquisitionMode.SetValue(AcquisitionModeEnums::AcquisitionMode_Continuous);

	/*
	* Set pixel format to Mono8
	*/
	m_camera->PixelFormat.SetValue(PixelFormatEnums::PixelFormat_Mono8);

	/*
	* Set region of interest
	*/
	// Offset x to minimum
	m_settings.roi.left = m_camera->OffsetX.GetMin();
	m_camera->OffsetX.SetValue(m_settings.roi.left);
	// Offset y to minimum
	m_settings.roi.top = m_camera->OffsetY.GetMin();
	m_camera->OffsetY.SetValue(m_settings.roi.top);
	// Width to maximum
	m_settings.roi.width = m_camera->Width.GetMax();
	m_camera->Width.SetValue(m_settings.roi.width);
	// Height to maximum
	m_settings.roi.height = m_camera->Height.GetMax();
	m_camera->Height.SetValue(m_settings.roi.height);
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