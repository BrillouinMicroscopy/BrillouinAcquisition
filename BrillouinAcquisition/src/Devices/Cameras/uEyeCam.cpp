#include "stdafx.h"
#include "uEyeCam.h"

/*
 * Public definitions
 */

uEyeCam::~uEyeCam() {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	disconnectDevice();
}

/*
 * Public slots
 */

void uEyeCam::connectDevice() {
	if (!m_isConnected) {

		// Select camera number
		m_camera = (uEye::HCAM)m_cameraNumber;

		auto nRet = uEye::is_InitCamera(&m_camera, NULL);
		if (nRet == IS_STARTER_FW_UPLOAD_NEEDED) {
			// Time for the firmware upload = 25 seconds by default
			auto nUploadTime{ 25000 };
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
			m_settings.roi.width_physical = 3840;
			m_settings.roi.height_physical = 2748;
			
			m_settings.roi.left = 800;
			m_settings.roi.top = 400;
			m_settings.roi.width_physical = 1800;
			m_settings.roi.height_physical = 2000;

			m_settings.readout.triggerMode = L"Software";
			m_settings.readout.pixelEncoding = L"Raw8";

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

void uEyeCam::stopPreview() {
	uEye::is_StopLiveVideo(m_camera, IS_FORCE_VIDEO_STOP);
	uEye::is_ClearSequence(m_camera);
	auto nRet = uEye::is_FreeImageMem(m_camera, m_imageBuffer, m_imageBufferId);
	m_isPreviewRunning = false;
	m_stopPreview = false;
	emit(s_previewRunning(m_isPreviewRunning));
}

void uEyeCam::startAcquisition(const CAMERA_SETTINGS& settings) {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	// check if currently a preview is running and stop it in case
	if (m_isPreviewRunning) {
		stopPreview();
		m_wasPreviewRunning = true;
	} else {
		m_wasPreviewRunning = false;
	}

	setSettings(settings);

	// Wait for camera to really apply settings
	Sleep(500);

	auto bufferSettings = BUFFER_SETTINGS{ 1, (unsigned int)m_settings.roi.bytesPerFrame, "unsigned char", m_settings.roi };
	m_previewBuffer->initializeBuffer(bufferSettings);
	emit(s_previewBufferSettingsChanged());

	// allocate and add memory
	auto nRet = uEye::is_AllocImageMem(m_camera, m_settings.roi.width_physical, m_settings.roi.height_physical, 8, &m_imageBuffer, &m_imageBufferId);

	if (nRet == IS_SUCCESS) {
		nRet = uEye::is_SetImageMem(m_camera, m_imageBuffer, m_imageBufferId);
	}

	m_isAcquisitionRunning = true;
	emit(s_acquisitionRunning(m_isAcquisitionRunning));
}

void uEyeCam::stopAcquisition() {
	auto nRet = uEye::is_FreeImageMem(m_camera, m_imageBuffer, m_imageBufferId);
	m_isAcquisitionRunning = false;
	emit(s_acquisitionRunning(m_isAcquisitionRunning));

	// Restart the preview if it was running before the acquisition
	if (m_wasPreviewRunning) {
		startPreview();
	}
}

void uEyeCam::getImageForAcquisition(std::byte* buffer, bool preview) {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	// Calculate timeout in multiples of 10 ms to be slightly higher than exposure time
	auto timeout{ (int)(500 * m_settings.exposureTime) };
	if (timeout < 20) {
		timeout = 20;
	}
	uEye::is_FreezeVideo(m_camera, timeout);
	acquireImage(buffer);

	if (preview && buffer != nullptr) {
		// write image to preview buffer
		memcpy(m_previewBuffer->m_buffer->getWriteBuffer(), buffer, m_settings.roi.bytesPerFrame);
		m_previewBuffer->m_buffer->m_usedBuffers->release();
		emit(s_imageReady());
	}
}

/*
 * Private definitions
 */

int uEyeCam::acquireImage(std::byte* buffer) {

	// Copy data to provided buffer
	if (m_imageBuffer != NULL && buffer != nullptr) {
		memcpy(buffer, m_imageBuffer, m_settings.roi.bytesPerFrame);
		return 1;
	}
	return 0;
}

void uEyeCam::readOptions() {

	m_options.pixelEncodings = { L"Raw8", L"Mono8" };
	////m_options.cycleModes = { L"Continuous", L"Fixed single", L"Fixed multiple" };

	auto exposureMin{ 0.0 };
	auto ret = uEye::is_Exposure(m_camera, uEye::IS_EXPOSURE_CMD_GET_EXPOSURE_RANGE_MIN, (void*)&exposureMin, sizeof(exposureMin));
	auto exposureMax{ 0.0 };
	ret = uEye::is_Exposure(m_camera, uEye::IS_EXPOSURE_CMD_GET_EXPOSURE_RANGE_MAX, (void*)&exposureMax, sizeof(exposureMax));
	m_options.exposureTimeLimits[0] = 1e-3*exposureMin;
	m_options.exposureTimeLimits[1] = 1e-3*exposureMax;

	//auto sizeAOImin = uEye::IS_SIZE_2D{};
	//ret = uEye::is_AOI(m_camera, IS_AOI_IMAGE_GET_SIZE_MIN, (void*)&sizeAOImin, sizeof(sizeAOImin));
	//auto sizeAOImax = uEye::IS_SIZE_2D{};
	//ret = uEye::is_AOI(m_camera, IS_AOI_IMAGE_GET_SIZE_MAX, (void*)&sizeAOImax, sizeof(sizeAOImax));

	auto sensorInfo = uEye::SENSORINFO{};
	ret = uEye::is_GetSensorInfo(m_camera, &sensorInfo);

	m_options.ROIWidthLimits[0] = 1;
	m_options.ROIWidthLimits[1] = sensorInfo.nMaxWidth;
	m_options.ROIHeightLimits[0] = 1;
	m_options.ROIHeightLimits[1] = sensorInfo.nMaxHeight;

	emit(optionsChanged(m_options));
}

void uEyeCam::readSettings() {

	/*
	 * Get the exposure time
	 */
	auto exposureTemp{ 0.0 };
	auto ret = uEye::is_Exposure(m_camera, uEye::IS_EXPOSURE_CMD_GET_EXPOSURE, (void*)&exposureTemp, sizeof(exposureTemp));
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
	auto AOI = uEye::IS_RECT{};
	ret = uEye::is_AOI(m_camera, IS_AOI_IMAGE_GET_AOI, (void*)&AOI, sizeof(AOI));
	if (ret == IS_SUCCESS) {
		m_settings.roi.height_physical = AOI.s32Height;
		m_settings.roi.width_physical = AOI.s32Width;
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

void uEyeCam::applySettings(const CAMERA_SETTINGS& settings) {
	m_settings = settings;

	// If the preview is currently running, stop it and apply the settings.
	if (m_isPreviewRunning) {
		uEye::is_StopLiveVideo(m_camera, IS_FORCE_VIDEO_STOP);
	}

	/*
	 * Set the exposure time
	 */
	auto exposureTemp = (double)1e3 * m_settings.exposureTime;
	uEye::is_Exposure(m_camera, uEye::IS_EXPOSURE_CMD_SET_EXPOSURE, (void*)&exposureTemp, sizeof(exposureTemp));

	/*
	 * Set the camera gain
	 */
	 //m_settings.gain;


	 /*
	  * Set the pixel format
	  */
	  // Set the pixel format, possible values are: PIXEL_FORMAT_RAW8, PIXEL_FORMAT_MONO8, PIXEL_FORMAT_MONO12, PIXEL_FORMAT_MONO16
	auto pixelFormat{ IS_CM_SENSOR_RAW8 };
	if (m_settings.readout.pixelEncoding == L"Raw8") {
		m_settings.readout.dataType = "unsigned char";
		pixelFormat = IS_CM_SENSOR_RAW8;
	} else if (m_settings.readout.pixelEncoding == L"Mono8") {
		m_settings.readout.dataType = "unsigned char";
		pixelFormat = IS_CM_MONO8;
	} else if (m_settings.readout.pixelEncoding == L"Mono12") {
		m_settings.readout.dataType = "unsigned short";
		pixelFormat = IS_CM_MONO12;
	} else if (m_settings.readout.pixelEncoding == L"Mono16") {
		m_settings.readout.dataType = "unsigned short";
		pixelFormat = IS_CM_MONO16;
	} else {
		m_settings.readout.pixelEncoding = L"Raw8";
		m_settings.readout.dataType = "unsigned char";
		pixelFormat = IS_CM_SENSOR_RAW8;
	}
	auto ret = uEye::is_SetColorMode(m_camera, pixelFormat);

	/*
	 * Set the region of interest
	 */
	auto AOI = uEye::IS_RECT{};
	// Offset x
	AOI.s32X = m_settings.roi.left;
	// Offset y
	AOI.s32Y = m_settings.roi.top;
	// Width
	AOI.s32Width = m_settings.roi.width_physical;
	// Height
	AOI.s32Height = m_settings.roi.height_physical;
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

	// Read changed options
	readOptions();

	m_settings.roi.width_binned = m_settings.roi.width_physical;
	m_settings.roi.height_binned = m_settings.roi.height_physical;

	m_settings.roi.bottom = m_options.ROIHeightLimits[1] - m_settings.roi.top - m_settings.roi.height_physical + 2;
	m_settings.roi.right = m_options.ROIWidthLimits[1] - m_settings.roi.left - m_settings.roi.width_physical + 2;

	m_settings.roi.bytesPerFrame = m_settings.roi.width_binned * m_settings.roi.height_binned;

	// If the preview was running, start it again.
	if (m_isPreviewRunning) {
		uEye::is_CaptureVideo(m_camera, IS_WAIT);
	}
}

void uEyeCam::preparePreview() {
	// set ROI and readout parameters to default preview values, exposure time and gain will be kept
	m_settings.roi.left = 0;
	m_settings.roi.top = 0;
	m_settings.roi.width_physical = m_options.ROIWidthLimits[1];
	m_settings.roi.height_physical = m_options.ROIHeightLimits[1];

	m_settings.roi.left = 800;
	m_settings.roi.top = 400;
	m_settings.roi.width_physical = 1800;
	m_settings.roi.height_physical = 2000;

	m_settings.readout.pixelEncoding = L"Raw8";
	m_settings.readout.triggerMode = L"Internal";

	setSettings(m_settings);

	auto bufferSettings = BUFFER_SETTINGS{ 1, (unsigned int)m_settings.roi.bytesPerFrame, "unsigned char", m_settings.roi };
	m_previewBuffer->initializeBuffer(bufferSettings);
	emit(s_previewBufferSettingsChanged());

	// allocate and add memory
	auto nRet = uEye::is_AllocImageMem(m_camera, m_settings.roi.width_physical, m_settings.roi.height_physical, 8, &m_imageBuffer, &m_imageBufferId);

	if (nRet == IS_SUCCESS) {
		nRet = uEye::is_AddToSequence(m_camera, m_imageBuffer, m_imageBufferId);
	}

	uEye::is_CaptureVideo(m_camera, IS_WAIT);
}