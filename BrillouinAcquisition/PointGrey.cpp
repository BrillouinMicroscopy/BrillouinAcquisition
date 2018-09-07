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

			// Initialize camera
			m_camera->Init();

			m_isConnected = true;
			
			readOptions();
			readSettings();

		} else {
			m_cameraList.Clear();
			// Release system
			m_system->ReleaseInstance();
		}
	}

	emit(connectedDevice(m_isConnected));
}

void PointGrey::disconnectDevice() {
	if (m_isConnected) {
		if (m_isPreviewRunning) {
			stopPreview();
		}

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

void PointGrey::readOptions() {

	m_options.pixelEncodings = { L"Mono8", L"Mono12 Packed", L"Mono12 Packed (IIDC-msb)", L"Mono16" };
	m_options.cycleModes = { L"Continuous", L"Fixed single", L"Fixed multiple" };

	m_options.exposureTimeLimits[0] = 1e-6*m_camera->ExposureTime.GetMin();
	m_options.exposureTimeLimits[1] = 1e-6*m_camera->ExposureTime.GetMax();

	int64_t tmp1 = m_camera->WidthMax();
	int64_t tmp2 = m_camera->WidthMax.GetMax();

	m_options.ROIWidthLimits[0] = 0;
	m_options.ROIWidthLimits[1] = m_camera->WidthMax();

	m_options.ROIHeightLimits[0] = 0;
	m_options.ROIHeightLimits[1] = m_camera->HeightMax();

	emit(optionsChanged(m_options));
}

CAMERA_SETTINGS PointGrey::readSettings() {
	// general settings
	m_settings.exposureTime = 1e-6*m_camera->ExposureTime.GetValue();

	//// ROI
	m_settings.roi.height = m_camera->Height.GetValue();
	m_settings.roi.width = m_camera->Width.GetValue();
	m_settings.roi.left = m_camera->OffsetX.GetValue();
	m_settings.roi.top = m_camera->OffsetY.GetValue();

	//// readout parameters
	PixelFormatEnums pixelFormat = m_camera->PixelFormat.GetValue();
	switch (pixelFormat) {
		case PixelFormat_Mono8:
			m_settings.readout.pixelEncoding = L"Mono8";
			break;
		case PixelFormat_Mono12p:
			m_settings.readout.pixelEncoding = L"Mono12 Packed";
			break;
		case PixelFormat_Mono12Packed:
			m_settings.readout.pixelEncoding = L"Mono12 Packed (IIDC-msb)";
			break;
		case PixelFormat_Mono16:
			m_settings.readout.pixelEncoding = L"Mono16";
			break;
	}

	AcquisitionModeEnums cycleMode = m_camera->AcquisitionMode.GetValue();
	switch (cycleMode) {
		case AcquisitionMode_Continuous:
			m_settings.readout.cycleMode = L"Continuous";
			break;
		case AcquisitionMode_SingleFrame:
			m_settings.readout.cycleMode = L"Fixed single";
			break;
		case AcquisitionMode_MultiFrame:
			m_settings.readout.cycleMode = L"Fixed multiple";
			break;
	}

	// emit signal that settings changed
	emit(settingsChanged(m_settings));

	return m_settings;
};

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
	// Possible values: PixelFormat_Mono8, PixelFormat_Mono12p, PixelFormat_Mono12packed, PixelFormat_Mono16 
	m_camera->PixelFormat.SetValue(PixelFormatEnums::PixelFormat_Mono8);

	/*
	 * Set the exposure time
	 */
	m_camera->ExposureAuto.SetValue(ExposureAutoEnums::ExposureAuto_Off);
	m_camera->ExposureMode.SetValue(ExposureModeEnums::ExposureMode_Timed);

	// exposure time in microseconds
	m_settings.exposureTime = 0.001;
	m_camera->ExposureTime.SetValue(1e6*m_settings.exposureTime);

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

	m_camera->TriggerMode.SetValue(TriggerModeEnums::TriggerMode_Off);

	m_camera->TriggerSource.SetValue(TriggerSourceEnums::TriggerSource_Software);

	m_camera->TriggerMode.SetValue(TriggerModeEnums::TriggerMode_On);

	readSettings();
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

	m_camera->TriggerSoftware();

	ImagePtr pResultImage = m_camera->GetNextImage();

	if (!pResultImage->IsIncomplete()) {
		ImagePtr convertedImage = pResultImage->Convert(PixelFormat_Mono8);

		unsigned char* data = static_cast<unsigned char*>(convertedImage->GetData());

		memcpy(buffer, data, m_settings.roi.width*m_settings.roi.height);
	}

	pResultImage->Release();
};