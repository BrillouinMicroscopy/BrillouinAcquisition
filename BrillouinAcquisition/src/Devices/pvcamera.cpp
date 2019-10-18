#include "stdafx.h"
#include "pvcamera.h"

PVCamera::~PVCamera() {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	disconnectDevice();
	if (m_isInitialised) {
		PVCam::pl_pvcam_uninit();
	}
}

bool PVCamera::initialize() {
	if (!m_isInitialised) {
		bool i_retCode = PVCam::pl_pvcam_init();
		if (i_retCode != PVCam::PV_OK) {
			//error condition
			//PrintErrorMessage(PVCam::pl_error_code(), "pl_pvcam_init() error");
			m_isInitialised = false;
		} else {
			PVCam::int16 i_numberOfCameras{ 0 };
			PVCam::pl_cam_get_total(&i_numberOfCameras);
			if (i_numberOfCameras > 0) {
				m_isInitialised = true;
			} else {
				emit(noCameraFound());
				PVCam::pl_pvcam_uninit();
				m_isInitialised = false;
			}
		}
	}
	return m_isInitialised;
}

void PVCamera::init() {
	// create timers and connect their signals
	// after moving camera to another thread
	m_tempTimer = new QTimer();
	QMetaObject::Connection connection = QWidget::connect(m_tempTimer, SIGNAL(timeout()), this, SLOT(checkSensorTemperature()));
}

void PVCamera::connectDevice() {
	// initialize library
	initialize();
	if (!m_isConnected && m_isInitialised) {
		char g_Camera0_Name[CAM_NAME_LEN] = "";
		PVCam::pl_cam_get_name(0, g_Camera0_Name);
		bool i_retCode = PVCam::pl_cam_open(g_Camera0_Name, &m_camera, PVCam::OPEN_EXCLUSIVE);
		if (i_retCode == PVCam::PV_OK) {
			m_isConnected = true;
			readOptions();
			setSettings(m_settings);
			if (!m_tempTimer->isActive()) {
				m_tempTimer->start(1000);
			}
		}
	}
	emit(connectedDevice(m_isConnected));
}

void PVCamera::disconnectDevice() {
	if (m_isConnected) {
		if (m_tempTimer->isActive()) {
			m_tempTimer->stop();
		}
		bool i_retCode = PVCam::pl_cam_close(m_camera);
		if (i_retCode == PVCam::PV_OK) {
			m_isConnected = false;
		}
	}
	emit(connectedDevice(m_isConnected));
}

void PVCamera::readOptions() {
	int i_retCode{ 0 };
	// Read min and max temperature setpoint
	PVCam::int16 setpointMin{ 0 };
	i_retCode = PVCam::pl_get_param(m_camera, PARAM_TEMP_SETPOINT, PVCam::ATTR_MIN, (void*)&setpointMin);
	if (i_retCode == PVCam::PV_OK) {
		m_sensorTemperature.minSetpoint = setpointMin / 100.0;
	}

	PVCam::int16 setpointMax{ 0 };
	PVCam::pl_get_param(m_camera, PARAM_TEMP_SETPOINT, PVCam::ATTR_MAX, (void*)&setpointMax);
	if (i_retCode == PVCam::PV_OK) {
		m_sensorTemperature.maxSetpoint = setpointMax / 100.0;
	}

	
	PVCam::int16 ROIHeight{ 0 };
	PVCam::pl_get_param(m_camera, PARAM_PAR_SIZE, PVCam::ATTR_CURRENT, (void*)&ROIHeight);
	m_options.ROIHeightLimits[0] = 0;
	m_options.ROIHeightLimits[1] = ROIHeight;

	PVCam::int16 ROIWidth{ 0 };
	PVCam::pl_get_param(m_camera, PARAM_SER_SIZE, PVCam::ATTR_CURRENT, (void*)&ROIWidth);
	m_options.ROIWidthLimits[0] = 0;
	m_options.ROIWidthLimits[1] = ROIWidth;

	//AT_GetFloatMin(m_camera, L"ExposureTime", &m_options.exposureTimeLimits[0]);
	//AT_GetFloatMax(m_camera, L"ExposureTime", &m_options.exposureTimeLimits[1]);
	//AT_GetIntMin(m_camera, L"FrameCount", &m_options.frameCountLimits[0]);
	//AT_GetIntMax(m_camera, L"FrameCount", &m_options.frameCountLimits[1]);

	emit(optionsChanged(m_options));
}

void PVCamera::setSettings(CAMERA_SETTINGS settings) {
	m_settings = settings;

	//// Set the pixel Encoding
	//AT_SetEnumeratedString(m_camera, L"Pixel Encoding", m_settings.readout.pixelEncoding.c_str());

	//// Set the pixel Readout Rate
	//AT_SetEnumeratedString(m_camera, L"Pixel Readout Rate", m_settings.readout.pixelReadoutRate.c_str());

	//// Set the exposure time
	//AT_SetFloat(m_camera, L"ExposureTime", m_settings.exposureTime);

	//// enable spurious noise filter
	//AT_SetBool(m_camera, L"SpuriousNoiseFilter", m_settings.spuriousNoiseFilter);

	//// Set the AOI
	//AT_SetInt(m_camera, L"AOIWidth", m_settings.roi.width);
	//AT_SetInt(m_camera, L"AOILeft", m_settings.roi.left);
	//AT_SetInt(m_camera, L"AOIHeight", m_settings.roi.height);
	//AT_SetInt(m_camera, L"AOITop", m_settings.roi.top);
	//AT_SetEnumeratedString(m_camera, L"AOIBinning", m_settings.roi.binning.c_str());
	//AT_SetEnumeratedString(m_camera, L"SimplePreAmpGainControl", m_settings.readout.preAmpGain.c_str());

	//AT_SetEnumeratedString(m_camera, L"CycleMode", m_settings.readout.cycleMode.c_str());
	//AT_SetEnumeratedString(m_camera, L"TriggerMode", m_settings.readout.triggerMode.c_str());

	//// Allocate a buffer
	//// Get the number of bytes required to store one frame
	//AT_64 ImageSizeBytes;
	//AT_GetInt(m_camera, L"ImageSizeBytes", &ImageSizeBytes);
	//m_bufferSize = static_cast<int>(ImageSizeBytes);

	//AT_GetInt(m_camera, L"AOIHeight", &m_settings.roi.height);
	//AT_GetInt(m_camera, L"AOIWidth", &m_settings.roi.width);
	//AT_GetInt(m_camera, L"AOILeft", &m_settings.roi.left);
	//AT_GetInt(m_camera, L"AOITop", &m_settings.roi.top);

	// read back the settings
	readSettings();
}

PVCam::rgn_type PVCamera::getCamSettings() {
	PVCam::rgn_type camSettings;
	camSettings.s1 = m_settings.roi.left - 1;
	camSettings.s2 = m_settings.roi.width + m_settings.roi.left - 2;
	camSettings.p1 = m_settings.roi.top - 1;
	camSettings.p2 = m_settings.roi.height + m_settings.roi.top - 2;
	int binning{ 1 };
	if (m_settings.roi.binning == L"4x4") {
		binning = 4;
	}
	else if (m_settings.roi.binning == L"2x2") {
		binning = 2;
	}
	camSettings.sbin = binning;
	camSettings.pbin = binning;
	return camSettings;
}

void PVCamera::readSettings() {
	//// general settings
	//AT_GetFloat(m_camera, L"ExposureTime", &m_settings.exposureTime);
	////AT_GetInt(m_camera, L"FrameCount", &m_settings.frameCount);
	//AT_GetBool(m_camera, L"SpuriousNoiseFilter", (int*)&m_settings.spuriousNoiseFilter);

	//// ROI
	//AT_GetInt(m_camera, L"AOIHeight", &m_settings.roi.height);
	//AT_GetInt(m_camera, L"AOIWidth", &m_settings.roi.width);
	//AT_GetInt(m_camera, L"AOILeft", &m_settings.roi.left);
	//AT_GetInt(m_camera, L"AOITop", &m_settings.roi.top);
	//getEnumString(L"AOIBinning", &m_settings.roi.binning);

	//// readout parameters
	//getEnumString(L"CycleMode", &m_settings.readout.cycleMode);
	//getEnumString(L"Pixel Encoding", &m_settings.readout.pixelEncoding);
	//getEnumString(L"Pixel Readout Rate", &m_settings.readout.pixelReadoutRate);
	//getEnumString(L"SimplePreAmpGainControl", &m_settings.readout.preAmpGain);
	//getEnumString(L"TriggerMode", &m_settings.readout.triggerMode);

	// emit signal that settings changed
	emit(settingsChanged(m_settings));
}

void PVCamera::setSensorCooling(bool cooling) {
	if (cooling) {
		//m_sensorTemperature.setpoint = m_sensorTemperature.minSetpoint;
		// TODO: We set the temperature to -15 °C for now, so we don't stress
		// the camera to much while developing.
		m_sensorTemperature.setpoint = -15.0;
	} else {
		double setpoint = m_sensorTemperature.maxSetpoint;
		// We want to set the value no higher than room temperature.
		if (setpoint > 20) {
			setpoint = 20;
		}
		m_sensorTemperature.setpoint = setpoint;
	}
	PVCam::int16 setpoint = 100 * m_sensorTemperature.setpoint;
	int i_retCode = PVCam::pl_set_param(m_camera, PARAM_TEMP_SETPOINT, (void*)&setpoint);
	m_isCooling = cooling;
	emit(cameraCoolingChanged(m_isCooling));
}

bool PVCamera::getSensorCooling() {
	PVCam::int16 setpoint{ 0 };
	int i_retCode = PVCam::pl_get_param(m_camera, PARAM_TEMP_SETPOINT, PVCam::ATTR_CURRENT, (void*)&setpoint);
	// If the setpoint is lower than 0 °C we consider it cooling.
	if (setpoint / 100.0 < 0.0) {
		return true;
	} else {
		return false;
	}
}

const std::string PVCamera::getTemperatureStatus() {
	// If the setpoint is above 0, we consider it not cooling
	if (m_sensorTemperature.setpoint >= 0) {
		return "Cooler Off";
	}
	// If sensor temperature and setpoint differ no more than 1 °C
	// we consider the temperature stabilised.
	double temp = getSensorTemperature();
	if (abs(temp - m_sensorTemperature.setpoint) < 1.0) {
		return "Stabilised";
	}
	return "Cooling";
}

double PVCamera::getSensorTemperature() {
	PVCam::int16 temperature;
	bool i_retCode = PVCam::pl_get_param(m_camera, PARAM_TEMP, PVCam::ATTR_CURRENT, (void*)&temperature);
	return temperature / 100.0;
}

void PVCamera::checkSensorTemperature() {
	m_sensorTemperature.temperature = getSensorTemperature();
	std::string status = getTemperatureStatus();
	if (status == "Cooler Off") {
		m_sensorTemperature.status = COOLER_OFF;
	} else if (status == "Cooling") {
		m_sensorTemperature.status = COOLING;
	} else if (status == "Stabilised") {
		m_sensorTemperature.status = STABILISED;
	} else {
		m_sensorTemperature.status = FAULT;
	}
	emit(s_sensorTemperatureChanged(m_sensorTemperature));
}

void PVCamera::startPreview() {
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

void PVCamera::preparePreview() {
	// always use full camera image for live preview
	m_settings.roi.width = m_options.ROIWidthLimits[1];
	m_settings.roi.left = 1;
	m_settings.roi.height = m_options.ROIHeightLimits[1];
	m_settings.roi.top = 1;

	setSettings(m_settings);

	PVCam::rgn_type settings = getCamSettings();

	int circBufferFrames{ 5 };
	PVCam::uns32 bufferSize;
	bool i_retCode = PVCam::pl_exp_setup_cont(m_camera, 1, &settings, PVCam::TIMED_MODE, 1e3 * m_settings.exposureTime,
		&bufferSize, PVCam::CIRC_NO_OVERWRITE);
	m_bufferSize = bufferSize;

	// preview buffer
	BUFFER_SETTINGS bufferSettings = { circBufferFrames, bufferSize, "unsigned short", m_settings.roi };
	m_previewBuffer->initializeBuffer(bufferSettings);
	emit(s_previewBufferSettingsChanged());

	// internal buffer
	if (m_buffer) {
		delete[] m_buffer;
		m_buffer = nullptr;
	}
	int bufSize = circBufferFrames * bufferSize / sizeof(PVCam::uns16);
	m_buffer = new (std::nothrow) PVCam::uns16[bufSize];

	// Start acquisition
	PVCam::pl_exp_start_cont(m_camera, m_buffer, bufSize);
}

void PVCamera::stopPreview() {
	cleanupAcquisition();
	m_isPreviewRunning = false;
	m_stopPreview = false;
	emit(s_previewRunning(m_isPreviewRunning));
}

void PVCamera::startAcquisition(CAMERA_SETTINGS settings) {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	// check if currently a preview is running and stop it in case
	if (m_isPreviewRunning) {
		stopPreview();
	}

	setSettings(settings);

	//AT_64 ImageSizeBytes;
	//AT_GetInt(m_camera, L"ImageSizeBytes", &ImageSizeBytes);
	//int BufferSize = static_cast<int>(ImageSizeBytes);

	//BUFFER_SETTINGS bufferSettings = { 4, BufferSize, "unsigned short", m_settings.roi };
	//m_previewBuffer->initializeBuffer(bufferSettings);
	//emit(s_previewBufferSettingsChanged());

	//// Start acquisition
	//AT_Command(m_camera, L"AcquisitionStart");
	//AT_InitialiseUtilityLibrary();

	m_isAcquisitionRunning = true;
	emit(s_acquisitionRunning(m_isAcquisitionRunning));
}

void PVCamera::stopAcquisition() {
	cleanupAcquisition();
	m_isAcquisitionRunning = false;
	emit(s_acquisitionRunning(m_isAcquisitionRunning));
}

void PVCamera::cleanupAcquisition() {
	PVCam::pl_exp_stop_cont(m_camera, PVCam::CCS_CLEAR);
}

void PVCamera::acquireImage(unsigned char* buffer) {
	PVCam::uns16* frameAddress;
	PVCam::pl_exp_get_latest_frame(m_camera, (void**)&frameAddress);

	memcpy(buffer, frameAddress, m_bufferSize);


	//// Pass this buffer to the SDK
	//unsigned char* UserBuffer = new unsigned char[m_bufferSize];
	//AT_QueueBuffer(m_camera, UserBuffer, m_bufferSize);

	//// Acquire camera images
	//AT_Command(m_camera, L"SoftwareTrigger");

	//// Sleep in this thread until data is ready
	//unsigned char* Buffer;
	//int ret = AT_WaitBuffer(m_camera, &Buffer, &m_bufferSize, 1500 * m_settings.exposureTime);
	//// return if AT_WaitBuffer timed out
	//if (ret == AT_ERR_TIMEDOUT) {
	//	return;
	//}

	//// Process the image
	////Unpack the 12 bit packed data
	//AT_GetInt(m_camera, L"AOIHeight", &m_settings.roi.height);
	//AT_GetInt(m_camera, L"AOIWidth", &m_settings.roi.width);
	//AT_GetInt(m_camera, L"AOIStride", &m_imageStride);

	//AT_ConvertBuffer(Buffer, buffer, m_settings.roi.width, m_settings.roi.height, m_imageStride, m_settings.readout.pixelEncoding.c_str(), L"Mono16");

	//delete[] Buffer;
}

void PVCamera::getImageForAcquisition(unsigned char* buffer, bool preview) {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	acquireImage(buffer);

	if (preview) {
		// write image to preview buffer
		memcpy(m_previewBuffer->m_buffer->getWriteBuffer(), buffer, m_settings.roi.width * m_settings.roi.height * 2);
		m_previewBuffer->m_buffer->m_usedBuffers->release();
	}
}

void PVCamera::setCalibrationExposureTime(double exposureTime) {
	//m_settings.exposureTime = exposureTime;
	//AT_Command(m_camera, L"AcquisitionStop");
	//// Set the exposure time
	//AT_SetFloat(m_camera, L"ExposureTime", m_settings.exposureTime);

	//AT_Command(m_camera, L"AcquisitionStart");
}
