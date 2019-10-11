#include "stdafx.h"
#include "pvcamera.h"

PVCamera::~PVCamera() {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	disconnectDevice();
}

bool PVCamera::initialize() {
	if (!m_isInitialised) {
	}
	return m_isInitialised;
}

void PVCamera::init() {
	// create timers and connect their signals
	// after moving andor to another thread
	m_tempTimer = new QTimer();
	QMetaObject::Connection connection = QWidget::connect(m_tempTimer, SIGNAL(timeout()), this, SLOT(checkSensorTemperature()));
}

void PVCamera::connectDevice() {
	// initialize library
	initialize();
	if (!m_isConnected && m_isInitialised) {
	}
	emit(connectedDevice(m_isConnected));
}

void PVCamera::disconnectDevice() {
	if (m_isConnected) {
		if (m_tempTimer->isActive()) {
			m_tempTimer->stop();
		}
		//int i_retCode = AT_Close(m_camera);
		//if (i_retCode == AT_SUCCESS) {
		//	m_isConnected = false;
		//}
	}
	emit(connectedDevice(m_isConnected));
}

void PVCamera::readOptions() {

	//AT_GetFloatMin(m_camera, L"ExposureTime", &m_options.exposureTimeLimits[0]);
	//AT_GetFloatMax(m_camera, L"ExposureTime", &m_options.exposureTimeLimits[1]);
	//AT_GetIntMin(m_camera, L"FrameCount", &m_options.frameCountLimits[0]);
	//AT_GetIntMax(m_camera, L"FrameCount", &m_options.frameCountLimits[1]);

	//AT_GetIntMin(m_camera, L"AOIHeight", &m_options.ROIHeightLimits[0]);
	//AT_GetIntMax(m_camera, L"AOIHeight", &m_options.ROIHeightLimits[1]);

	//AT_GetIntMin(m_camera, L"AOIWidth", &m_options.ROIWidthLimits[0]);
	//AT_GetIntMax(m_camera, L"AOIWidth", &m_options.ROIWidthLimits[1]);

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
	//int i_retCode = AT_SetBool(m_camera, L"SensorCooling", (int)cooling);
	//m_isCooling = cooling;
	//emit(cameraCoolingChanged(m_isCooling));
}

bool PVCamera::getSensorCooling() {
	//AT_BOOL szValue;
	//int i_retCode = AT_GetBool(m_camera, L"SensorCooling", &szValue);
	//return szValue;
	return false;
}

const std::string PVCamera::getTemperatureStatus() {
	//int i_retCode = AT_GetEnumIndex(m_camera, L"TemperatureStatus", &m_temperatureStatusIndex);
	//AT_WC temperatureStatus[256];
	//AT_GetEnumStringByIndex(m_camera, L"TemperatureStatus", m_temperatureStatusIndex, temperatureStatus, 256);
	//std::wstring temperatureStatusString = temperatureStatus;
	//m_temperatureStatus = std::string(temperatureStatusString.begin(), temperatureStatusString.end());
	//return m_temperatureStatus;
	return "";
}

double PVCamera::getSensorTemperature() {
	//double szValue;
	//int i_retCode = AT_GetFloat(m_camera, L"SensorTemperature", &szValue);
	//return szValue;
	return 0.0;
}

void PVCamera::checkSensorTemperature() {
	//m_sensorTemperature.temperature = getSensorTemperature();
	//std::string status = getTemperatureStatus();
	//if (status == "Cooler Off") {
	//	m_sensorTemperature.status = COOLER_OFF;
	//}
	//else if (status == "Fault") {
	//	m_sensorTemperature.status = FAULT;
	//}
	//else if (status == "Cooling") {
	//	m_sensorTemperature.status = COOLING;
	//}
	//else if (status == "Drift") {
	//	m_sensorTemperature.status = DRIFT;
	//}
	//else if (status == "Not Stabilised") {
	//	m_sensorTemperature.status = NOT_STABILISED;
	//}
	//else if (status == "Stabilised") {
	//	m_sensorTemperature.status = STABILISED;
	//}
	//else {
	//	m_sensorTemperature.status = FAULT;
	//}
	//emit(s_sensorTemperatureChanged(m_sensorTemperature));
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

	//AT_64 ImageSizeBytes;
	//AT_GetInt(m_camera, L"ImageSizeBytes", &ImageSizeBytes);
	//int BufferSize = static_cast<int>(ImageSizeBytes);

	//BUFFER_SETTINGS bufferSettings = { 5, BufferSize, "unsigned short", m_settings.roi };
	//m_previewBuffer->initializeBuffer(bufferSettings);
	//emit(s_previewBufferSettingsChanged());

	//// Start acquisition
	//AT_Command(m_camera, L"AcquisitionStart");
	//AT_InitialiseUtilityLibrary();
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
	//AT_FinaliseUtilityLibrary();
	//AT_Command(m_camera, L"AcquisitionStop");
	//AT_Flush(m_camera);
}

void PVCamera::acquireImage(unsigned char* buffer) {
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
