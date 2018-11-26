#include "stdafx.h"
#include "uEyeCam.h"

uEyeCam::~uEyeCam() {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	disconnectDevice();
}

void uEyeCam::connectDevice() {
	if (!m_isConnected) {
		
		//unsigned int numCameras;
		//m_busManager.GetNumOfCameras(&numCameras);

		//if (numCameras > 0) {
		//	// Select camera

		//	m_busManager.GetCameraFromIndex(0, &m_guid);

		//	m_camera.Connect(&m_guid);

		//	m_isConnected = true;
		//	
		//	readOptions();

		//	// apply default values for exposure and gain
		//	m_settings.exposureTime = 0.004;
		//	m_settings.gain = 0.0;
		//	setSettings(m_settings);
		//}
	}

	m_isConnected = true;
	emit(connectedDevice(m_isConnected));
}

void uEyeCam::disconnectDevice() {
	if (m_isConnected) {
		if (m_isPreviewRunning) {
			stopPreview();
		}

		// Deinitialize camera
		if (m_camera != 0) {
			uEye::is_EnableMessage(m_camera, IS_FRAME, NULL);
			uEye::is_StopLiveVideo(m_camera, IS_WAIT);
			uEye::is_ExitCamera(m_camera);
			m_camera = NULL;
		}

		m_isConnected = false;
	}

	emit(connectedDevice(m_isConnected));
}

void uEyeCam::readOptions() {

	//FlyCapture2::Format7Info fmt7Info;
	//bool supported;
	//fmt7Info.mode = FlyCapture2::MODE_0;
	//m_camera.GetFormat7Info(&fmt7Info, &supported);

	//m_options.pixelEncodings = { L"Raw8", L"Mono8", L"Mono12", L"Mono16" };
	////m_options.cycleModes = { L"Continuous", L"Fixed single", L"Fixed multiple" };

	//m_options.exposureTimeLimits[0] = 1e-3;
	//m_options.exposureTimeLimits[1] = 1;

	//m_options.ROIWidthLimits[0] = 1;
	//m_options.ROIWidthLimits[1] = fmt7Info.maxWidth;

	//m_options.ROIHeightLimits[0] = 1;
	//m_options.ROIHeightLimits[1] = fmt7Info.maxHeight;

	emit(optionsChanged(m_options));
}

void uEyeCam::readSettings() {

	//// Get the camera configuration
	//FlyCapture2::FC2Config config;
	//m_camera.GetConfiguration(&config);

	///*
	//* Get the exposure time
	//*/
	//FlyCapture2::Property prop;
	////Define the property to adjust.
	//prop.type = FlyCapture2::SHUTTER;
	////Set the property.
	//m_camera.GetProperty(&prop);
	//// general settings
	//m_settings.exposureTime = 1e-3*prop.absValue;	// [s] exposure time

	///*
	// * Get the camera gain
	// */
	//FlyCapture2::Property propGain;
	////Define the property to adjust.
	//propGain.type = FlyCapture2::GAIN;
	////Get the property.
	//m_camera.GetProperty(&propGain);
	//// store property in settings
	//m_settings.gain = propGain.absValue;	// [dB] camera gain

	//// Create a Format7 Configuration
	//FlyCapture2::Format7ImageSettings fmt7ImageSettings;
	//unsigned int packetSize;
	//float speed;
	//m_camera.GetFormat7Configuration(&fmt7ImageSettings, &packetSize, &speed);

	//// ROI
	//m_settings.roi.height = fmt7ImageSettings.height;
	//m_settings.roi.width = fmt7ImageSettings.width;
	//m_settings.roi.left = fmt7ImageSettings.offsetX;
	//m_settings.roi.top = fmt7ImageSettings.offsetY;

	//// readout parameters
	//switch (fmt7ImageSettings.pixelFormat) {
	//	case FlyCapture2::PIXEL_FORMAT_RAW8 :
	//		m_settings.readout.pixelEncoding = L"Raw8";
	//		break;
	//	case FlyCapture2::PIXEL_FORMAT_MONO8:
	//		m_settings.readout.pixelEncoding = L"Mono8";
	//		break;
	//	case FlyCapture2::PIXEL_FORMAT_MONO12:
	//		m_settings.readout.pixelEncoding = L"Mono12";
	//		break;
	//	case FlyCapture2::PIXEL_FORMAT_MONO16:
	//		m_settings.readout.pixelEncoding = L"Mono16";
	//		break;
	//}

	// emit signal that settings changed
	emit(settingsChanged(m_settings));
}

void uEyeCam::setSettings(CAMERA_SETTINGS settings) {
	m_settings = settings;

	///*
	//* Set the exposure time
	//*/
	//FlyCapture2::Property prop;
	////Define the property to adjust.
	//prop.type = FlyCapture2::SHUTTER;
	////Ensure the property is on.
	//prop.onOff = true;
	//// Ensure auto - adjust mode is off.
	//prop.autoManualMode = false;
	////Ensure the property is set up to use absolute value control.
	//prop.absControl = true;
	////Set the absolute value of shutter
	//prop.absValue = 1e3*m_settings.exposureTime;
	////Set the property.
	//m_camera.SetProperty(&prop);


	///*
	// * Set the camera gain
	// */
	//FlyCapture2::Property propGain;
	//// Define the property to adjust.
	//propGain.type = FlyCapture2::GAIN;
	//// Ensure auto-adjust mode is off.
	//propGain.autoManualMode = false;
	//// Ensure the property is set up to use absolute value control.
	//propGain.absControl = true;
	////Set the absolute value of gain to 10.5 dB.
	//propGain.absValue = m_settings.gain;
	////Set the property.
	//m_camera.SetProperty(&propGain);


	///*
	//* Set region of interest and pixel format
	//*/
	//// Create a Format7 Configuration
	//FlyCapture2::Format7ImageSettings fmt7ImageSettings;
	//// Acquisition mode is always "MODE_0" for this application
	//fmt7ImageSettings.mode = FlyCapture2::MODE_0;
	//// Set the pixel format, possible values are: PIXEL_FORMAT_RAW8, PIXEL_FORMAT_MONO8, PIXEL_FORMAT_MONO12, PIXEL_FORMAT_MONO16
	//if (m_settings.readout.pixelEncoding == L"Raw8") {
	//	fmt7ImageSettings.pixelFormat = FlyCapture2::PIXEL_FORMAT_RAW8;
	//} else if (m_settings.readout.pixelEncoding == L"Mono8") {
	//	fmt7ImageSettings.pixelFormat = FlyCapture2::PIXEL_FORMAT_MONO8;
	//} else if (m_settings.readout.pixelEncoding == L"Mono12") {
	//	fmt7ImageSettings.pixelFormat = FlyCapture2::PIXEL_FORMAT_MONO12;
	//} else if (m_settings.readout.pixelEncoding == L"Mono16") {
	//	fmt7ImageSettings.pixelFormat = FlyCapture2::PIXEL_FORMAT_MONO16;
	//}

	//// Offset x
	//fmt7ImageSettings.offsetX = m_settings.roi.left;
	//// Offset y
	//fmt7ImageSettings.offsetY = m_settings.roi.top;
	//// Width
	//fmt7ImageSettings.width = m_settings.roi.width;
	//// Height
	//fmt7ImageSettings.height = m_settings.roi.height;

	//FlyCapture2::Format7PacketInfo fmt7PacketInfo;
	//bool valid;
	//m_camera.ValidateFormat7Settings(&fmt7ImageSettings, &valid, &fmt7PacketInfo);
	//if (valid) {
	//	m_camera.SetFormat7Configuration(&fmt7ImageSettings, fmt7PacketInfo.recommendedBytesPerPacket);
	//}

	///*
	//* Set trigger mode
	//*/
	//FlyCapture2::TriggerMode triggerMode;
	//m_camera.GetTriggerMode(&triggerMode);
	//triggerMode.mode = 0;
	//triggerMode.parameter = 0;
	//triggerMode.polarity = 0;
	//if (m_settings.readout.triggerMode == L"Internal") {
	//	triggerMode.onOff = false;
	//} else if (m_settings.readout.triggerMode == L"Software") {
	//	triggerMode.onOff = true;
	//	triggerMode.source = 7;	// 7 for software trigger
	//} else if (m_settings.readout.triggerMode == L"External") {
	//	triggerMode.onOff = true;
	//	triggerMode.source = 0;	// 0 for external trigger
	//}

	//m_camera.SetTriggerMode(&triggerMode);

	//// Wait for software trigger ready
	//if (m_settings.readout.triggerMode == L"Software") {
	//	PollForTriggerReady(&m_camera);
	//}

	///*
	//* Set the buffering mode.
	//*/
	//FlyCapture2::FC2Config BufferFrame;
	//m_camera.GetConfiguration(&BufferFrame);
	//if (m_settings.readout.cycleMode == L"Fixed") {				// For image preview
	//	BufferFrame.grabMode = FlyCapture2::DROP_FRAMES;
	//	BufferFrame.highPerformanceRetrieveBuffer = false;
	//} else if (m_settings.readout.cycleMode == L"Continuous") {	// For image preview
	//	BufferFrame.grabMode = FlyCapture2::BUFFER_FRAMES;
	//	BufferFrame.highPerformanceRetrieveBuffer = true;
	//}
	//BufferFrame.numBuffers = m_settings.frameCount;
	//m_camera.SetConfiguration(&BufferFrame);

	//// Read back the settings
	//readSettings();
}

void uEyeCam::startPreview() {
	// don't do anything if an acquisition is running
	if (m_isAcquisitionRunning) {
		return;
	}
	m_isPreviewRunning = true;
	m_stopPreview = false;
	preparePreview();
	getImageForPreview();

	emit(s_previewRunning(m_isPreviewRunning));
}

void uEyeCam::preparePreview() {
	// set ROI and readout parameters to default preview values, exposure time and gain will be kept
	m_settings.roi.left = 0;
	m_settings.roi.top = 0;
	m_settings.roi.width = m_options.ROIWidthLimits[1];
	m_settings.roi.height = m_options.ROIHeightLimits[1];
	m_settings.readout.pixelEncoding = L"Raw8";
	m_settings.readout.triggerMode = L"Internal";
	m_settings.readout.cycleMode = L"Fixed";
	m_settings.frameCount = 10;

	setSettings(m_settings);

	int pixelNumber = m_settings.roi.width * m_settings.roi.height;
	BUFFER_SETTINGS bufferSettings = { 4, pixelNumber, "unsigned char", m_settings.roi };
	m_previewBuffer->initializeBuffer(bufferSettings);
	emit(s_previewBufferSettingsChanged());

	//m_camera.StartCapture();
}

void uEyeCam::stopPreview() {
	//m_camera.StopCapture();
	m_isPreviewRunning = false;
	m_stopPreview = false;
	emit(s_previewRunning(m_isPreviewRunning));
}

void uEyeCam::startAcquisition(CAMERA_SETTINGS settings) {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	// check if currently a preview is running and stop it in case
	if (m_isPreviewRunning) {
		stopPreview();
	}

	setSettings(settings);

	// Wait for camera to really apply settings
	Sleep(500);

	//m_camera.StartCapture();
	m_isAcquisitionRunning = true;
	emit(s_acquisitionRunning(m_isAcquisitionRunning));
}

void uEyeCam::stopAcquisition() {
	//m_camera.StopCapture();
	m_isAcquisitionRunning = false;
	emit(s_acquisitionRunning(m_isAcquisitionRunning));
}

void uEyeCam::acquireImage(unsigned char* buffer) {
	//FlyCapture2::Image rawImage;
	//FlyCapture2::Error tmp = m_camera.RetrieveBuffer(&rawImage);

	//// Convert the raw image
	//FlyCapture2::Image convertedImage;
	//rawImage.Convert(FlyCapture2::PIXEL_FORMAT_RAW8, &convertedImage);

	//// Get access to raw data
	//T* data = static_cast<T*>(convertedImage.GetData());

	//// Copy data to preview buffer
	//if (data != NULL) {
	//	memcpy(buffer, data, m_settings.roi.width*m_settings.roi.height);
	//}
}

void uEyeCam::getImageForPreview() {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	if (m_isPreviewRunning) {
		if (m_stopPreview) {
			stopPreview();
			return;
		}

		m_previewBuffer->m_buffer->m_freeBuffers->acquire();
		acquireImage(m_previewBuffer->m_buffer->getWriteBuffer());
		m_previewBuffer->m_buffer->m_usedBuffers->release();

		QMetaObject::invokeMethod(this, "getImageForPreview", Qt::QueuedConnection);
	}
}

void uEyeCam::getImageForAcquisition(unsigned char* buffer) {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	acquireImage(buffer);
}