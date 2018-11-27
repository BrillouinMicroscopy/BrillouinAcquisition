#include "stdafx.h"
#include "uEyeCam.h"

uEyeCam::~uEyeCam() {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	disconnectDevice();
}

void uEyeCam::connectDevice() {
	if (!m_isConnected) {

		int nRet = uEye::is_InitCamera(&m_camera, NULL);
		if (nRet == IS_STARTER_FW_UPLOAD_NEEDED) {
			// Time for the firmware upload = 25 seconds by default
			int nUploadTime = 25000;
			uEye::is_GetDuration(m_camera, uEye::IS_STARTER_FW_UPLOAD, &nUploadTime);

			// Try again to open the camera. This time we allow the automatic upload of the firmware by
			// specifying "IS_ALLOW_STARTER_FIRMWARE_UPLOAD"
			m_camera = (uEye::HIDS)(((int)m_camera) | IS_ALLOW_STARTER_FW_UPLOAD);
			nRet = uEye::is_InitCamera(&m_camera, NULL);
		}

		if (nRet == IS_SUCCESS) {
			m_isConnected = true;

			readOptions();

			// apply default values for exposure and gain
			m_settings.exposureTime = 0.1;
			m_settings.gain = 0.0;

			m_settings.roi.left = 0;
			m_settings.roi.top = 0;
			m_settings.roi.width = 3840;
			m_settings.roi.height = 2748;

			m_settings.readout.triggerMode = L"Software";

			setSettings(m_settings);
		}
	}

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

	m_options.pixelEncodings = { L"Raw8", L"Mono8", L"Mono12", L"Mono16" };
	////m_options.cycleModes = { L"Continuous", L"Fixed single", L"Fixed multiple" };

	double exposureMin{ 0.0 };
	int ret = uEye::is_Exposure(m_camera, uEye::IS_EXPOSURE_CMD_GET_EXPOSURE_RANGE_MIN, (void*)&exposureMin, sizeof(exposureMin));
	double exposureMax{ 0.0 };
	ret = uEye::is_Exposure(m_camera, uEye::IS_EXPOSURE_CMD_GET_EXPOSURE_RANGE_MAX, (void*)&exposureMax, sizeof(exposureMax));
	m_options.exposureTimeLimits[0] = 1e-3*exposureMin;
	m_options.exposureTimeLimits[1] = 1e-3*exposureMax;

	uEye::IS_SIZE_2D sizeAOImin;
	ret = uEye::is_AOI(m_camera, IS_AOI_IMAGE_GET_SIZE_MIN, (void*)&sizeAOImin, sizeof(sizeAOImin));
	uEye::IS_SIZE_2D sizeAOImax;
	ret = uEye::is_AOI(m_camera, IS_AOI_IMAGE_GET_SIZE_MAX, (void*)&sizeAOImax, sizeof(sizeAOImax));
	m_options.ROIWidthLimits[0] = sizeAOImin.s32Width;
	m_options.ROIWidthLimits[1] = sizeAOImax.s32Width;
	m_options.ROIHeightLimits[0] = sizeAOImin.s32Height;
	m_options.ROIHeightLimits[1] = sizeAOImax.s32Height;

	emit(optionsChanged(m_options));
}

void uEyeCam::readSettings() {

	/*
	 * Get the exposure time
	 */
	double exposureTemp{ 0.0 };
	int ret = uEye::is_Exposure(m_camera, uEye::IS_EXPOSURE_CMD_GET_EXPOSURE, (void*)&exposureTemp, sizeof(exposureTemp));
	if (ret == IS_SUCCESS) {
		m_settings.exposureTime = 1e-3*exposureTemp;	// [s] exposure time
	}

	/*
	 * Get the camera gain
	 */
	//m_settings.gain;	// [dB] camera gain

	/*
	 * Get the region of interest
	 */
	uEye::IS_RECT AOI;
	ret = uEye::is_AOI(m_camera, IS_AOI_IMAGE_GET_AOI, (void*)&AOI, sizeof(AOI));
	if (ret == IS_SUCCESS) {
		m_settings.roi.height = AOI.s32Height;
		m_settings.roi.width = AOI.s32Width;
		m_settings.roi.left = AOI.s32X;
		m_settings.roi.top = AOI.s32Y;
	}

	/*
	 * Get the currently selected trigger mode
	 */

	ret = uEye::is_SetExternalTrigger(m_camera, IS_GET_EXTERNALTRIGGER);
	switch (ret) {
		case IS_SET_TRIGGER_OFF:
			m_settings.readout.triggerMode = L"Internal";
			break;
		case IS_SET_TRIGGER_SOFTWARE:
			m_settings.readout.triggerMode = L"Software";
			break;
		case IS_SET_TRIGGER_LO_HI:
			m_settings.readout.triggerMode = L"External";
			break;
	}

	// readout parameters
	int tmp = uEye::is_SetColorMode(m_camera, IS_GET_COLOR_MODE);
	switch (tmp) {
		case IS_CM_SENSOR_RAW8:
			m_settings.readout.pixelEncoding = L"Raw8";
			break;
		case IS_CM_MONO8:
			m_settings.readout.pixelEncoding = L"Mono8";
			break;
		case IS_CM_MONO12:
			m_settings.readout.pixelEncoding = L"Mono12";
			break;
		case IS_CM_MONO16:
			m_settings.readout.pixelEncoding = L"Mono16";
			break;
	}

	// emit signal that settings changed
	emit(settingsChanged(m_settings));
}

void uEyeCam::setSettings(CAMERA_SETTINGS settings) {
	m_settings = settings;

	/*
	 * Set the exposure time
	 */
	double exposureTemp{ 0.0 };
	exposureTemp = (double)1e3*m_settings.exposureTime;
	uEye::is_Exposure(m_camera, uEye::IS_EXPOSURE_CMD_SET_EXPOSURE, (void*)&exposureTemp, sizeof(exposureTemp));

	/*
	 * Set the camera gain
	 */
	//m_settings.gain;


	/*
	 * Set the pixel format
	 */
	// Set the pixel format, possible values are: PIXEL_FORMAT_RAW8, PIXEL_FORMAT_MONO8, PIXEL_FORMAT_MONO12, PIXEL_FORMAT_MONO16
	int pixelFormat{ IS_CM_SENSOR_RAW8 };
	if (m_settings.readout.pixelEncoding == L"Raw8") {
		pixelFormat = IS_CM_SENSOR_RAW8;
	} else if (m_settings.readout.pixelEncoding == L"Mono8") {
		pixelFormat = IS_CM_MONO8;
	} else if (m_settings.readout.pixelEncoding == L"Mono12") {
		pixelFormat = IS_CM_MONO12;
	} else if (m_settings.readout.pixelEncoding == L"Mono16") {
		pixelFormat = IS_CM_MONO16;
	}
	int ret = uEye::is_SetColorMode(m_camera, pixelFormat);

	/*
	 * Set the region of interest
	 */
	uEye::IS_RECT AOI;
	// Offset x
	AOI.s32X = m_settings.roi.left;
	// Offset y
	AOI.s32Y = m_settings.roi.top;
	// Width
	AOI.s32Width = m_settings.roi.width;
	// Height
	AOI.s32Height = m_settings.roi.height;
	// Apply values
	ret = uEye::is_AOI(m_camera, IS_AOI_IMAGE_SET_AOI, (void*)&AOI, sizeof(AOI));

	/*
	 * Set trigger mode
	 */
	if (m_settings.readout.triggerMode == L"Internal") {
		ret = uEye::is_SetExternalTrigger(m_camera, IS_SET_TRIGGER_OFF);
	} else if (m_settings.readout.triggerMode == L"Software") {
		ret = uEye::is_SetExternalTrigger(m_camera, IS_SET_TRIGGER_SOFTWARE);	// software trigger
	} else if (m_settings.readout.triggerMode == L"External") {
		ret = uEye::is_SetExternalTrigger(m_camera, IS_SET_TRIGGER_LO_HI);	// external trigger low high
	}

	// Read back the settings
	readSettings();
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