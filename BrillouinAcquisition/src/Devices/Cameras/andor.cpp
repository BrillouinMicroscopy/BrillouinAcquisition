#include "stdafx.h"
#include "andor.h"

/*
 * Public definitions
 */

Andor::~Andor() {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	disconnectDevice();
	if (m_isInitialised) {
		AT_FinaliseLibrary();
	}
}

/*
 * Public slots
 */

void Andor::init() {
	// create timers and connect their signals
	// after moving andor to another thread
	m_tempTimer = new QTimer();
	auto connection = QWidget::connect(
		m_tempTimer,
		&QTimer::timeout,
		this,
		&Andor::checkSensorTemperature
	);
}

void Andor::connectDevice() {
	// initialize library
	initialize();
	if (!m_isConnected && m_isInitialised) {
		auto i_retCode = AT_Open(0, &m_camera);
		if (i_retCode == AT_SUCCESS) {
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

void Andor::disconnectDevice() {
	if (m_isConnected) {
		if (m_tempTimer->isActive()) {
			m_tempTimer->stop();
		}
		auto i_retCode = AT_Close(m_camera);
		if (i_retCode == AT_SUCCESS) {
			m_isConnected = false;
		}
	}
	emit(connectedDevice(m_isConnected));
}

void Andor::setSettings(CAMERA_SETTINGS settings) {
	m_settings = settings;

	// Set the pixel Encoding
	AT_SetEnumeratedString(m_camera, L"Pixel Encoding", m_settings.readout.pixelEncoding.c_str());

	// Set the pixel Readout Rate
	AT_SetEnumeratedString(m_camera, L"Pixel Readout Rate", m_settings.readout.pixelReadoutRate.c_str());

	// Set the exposure time
	AT_SetFloat(m_camera, L"ExposureTime", m_settings.exposureTime);

	// enable spurious noise filter
	AT_SetBool(m_camera, L"SpuriousNoiseFilter", m_settings.spuriousNoiseFilter);

	// Set the AOI
	auto binning{ 1 };
	if (m_settings.roi.binning == L"8x8") {
		binning = 8;
	} else if (m_settings.roi.binning == L"4x4") {
		binning = 4;
	} else if (m_settings.roi.binning == L"2x2") {
		binning = 2;
	} else if (m_settings.roi.binning == L"1x1") {
		binning = 1;
	} else {
		// Fallback to 1x1 binning
		m_settings.roi.binning = L"1x1";
	}
	m_settings.roi.binX = binning;
	m_settings.roi.binY = binning;

	// Verify that the image size is a multiple of the binning number
	auto modx = m_settings.roi.width_physical % m_settings.roi.binX;
	if (modx) {
		m_settings.roi.width_physical -= modx;
	}
	auto mody = m_settings.roi.height_physical % m_settings.roi.binY;
	if (mody) {
		m_settings.roi.height_physical -= mody;
	}

	AT_SetEnumeratedString(m_camera, L"AOIBinning", m_settings.roi.binning.c_str());
	AT_SetInt(m_camera, L"AOIWidth", m_settings.roi.width_physical / m_settings.roi.binX);
	AT_SetInt(m_camera, L"AOILeft", m_settings.roi.left);
	AT_SetInt(m_camera, L"AOIHeight", m_settings.roi.height_physical / m_settings.roi.binY);
	AT_SetInt(m_camera, L"AOITop", m_settings.roi.top);
	AT_SetEnumeratedString(m_camera, L"SimplePreAmpGainControl", m_settings.readout.preAmpGain.c_str());

	AT_SetEnumeratedString(m_camera, L"CycleMode", m_settings.readout.cycleMode.c_str());
	AT_SetEnumeratedString(m_camera, L"TriggerMode", m_settings.readout.triggerMode.c_str());

	// Allocate a buffer
	// Get the number of bytes required to store one frame
	auto ImageSizeBytes = AT_64{};
	AT_GetInt(m_camera, L"ImageSizeBytes", &ImageSizeBytes);
	m_settings.roi.bytesPerFrame = static_cast<int>(ImageSizeBytes);

	// read back the settings
	readSettings();
}

void Andor::startPreview() {
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

void Andor::stopPreview() {
	cleanupAcquisition();
	m_isPreviewRunning = false;
	m_stopPreview = false;
	emit(s_previewRunning(m_isPreviewRunning));
}

void Andor::startAcquisition(CAMERA_SETTINGS settings) {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	// check if currently a preview is running and stop it in case
	if (m_isPreviewRunning) {
		stopPreview();
	}

	setSettings(settings);

	auto bufferSettings = BUFFER_SETTINGS{ 4, (unsigned int)m_settings.roi.bytesPerFrame, "unsigned short", m_settings.roi };
	m_previewBuffer->initializeBuffer(bufferSettings);
	emit(s_previewBufferSettingsChanged());

	// Start acquisition
	AT_Command(m_camera, L"AcquisitionStart");
	AT_InitialiseUtilityLibrary();

	m_isAcquisitionRunning = true;
	emit(s_acquisitionRunning(m_isAcquisitionRunning));
}

void Andor::stopAcquisition() {
	cleanupAcquisition();
	m_isAcquisitionRunning = false;
	emit(s_acquisitionRunning(m_isAcquisitionRunning));
}

void Andor::getImageForAcquisition(unsigned char* buffer, bool preview) {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	acquireImage(buffer);

	if (preview) {
		// write image to preview buffer
		memcpy(m_previewBuffer->m_buffer->getWriteBuffer(), buffer, m_settings.roi.bytesPerFrame);
		m_previewBuffer->m_buffer->m_usedBuffers->release();
	}
}

void Andor::setCalibrationExposureTime(double exposureTime) {
	m_settings.exposureTime = exposureTime;
	AT_Command(m_camera, L"AcquisitionStop");
	// Set the exposure time
	AT_SetFloat(m_camera, L"ExposureTime", m_settings.exposureTime);

	AT_Command(m_camera, L"AcquisitionStart");
}

void Andor::setSensorCooling(bool cooling) {
	auto i_retCode = AT_SetBool(m_camera, L"SensorCooling", (int)cooling);
	m_isCooling = cooling;
	emit(cameraCoolingChanged(m_isCooling));
}

bool Andor::getSensorCooling() {
	auto szValue = AT_BOOL{};
	auto i_retCode = AT_GetBool(m_camera, L"SensorCooling", &szValue);
	return szValue;
}

/*
 * Private definitions
 */

int Andor::acquireImage(unsigned char* buffer) {
	// Pass this buffer to the SDK
	auto UserBuffer = new unsigned char[m_settings.roi.bytesPerFrame];
	AT_QueueBuffer(m_camera, UserBuffer, m_settings.roi.bytesPerFrame);

	// Acquire camera images
	AT_Command(m_camera, L"SoftwareTrigger");

	// Sleep in this thread until data is ready
	unsigned char* Buffer{ nullptr };
	auto ret = AT_WaitBuffer(m_camera, &Buffer, &m_settings.roi.bytesPerFrame, 1500 * m_settings.exposureTime);
	// return if AT_WaitBuffer timed out
	if (ret == AT_ERR_TIMEDOUT) {
		return 0;
	}

	// Process the image
	//Unpack the 12 bit packed data
	AT_GetInt(m_camera, L"AOIHeight", &m_settings.roi.height_binned);
	AT_GetInt(m_camera, L"AOIWidth", &m_settings.roi.width_binned);
	AT_GetInt(m_camera, L"AOIStride", &m_imageStride);

	AT_ConvertBuffer(Buffer, buffer, m_settings.roi.width_binned, m_settings.roi.height_binned, m_imageStride, m_settings.readout.pixelEncoding.c_str(), L"Mono16");

	delete[] Buffer;
	return 1;
}

void Andor::readOptions() {

	AT_GetFloatMin(m_camera, L"ExposureTime", &m_options.exposureTimeLimits[0]);
	AT_GetFloatMax(m_camera, L"ExposureTime", &m_options.exposureTimeLimits[1]);
	AT_GetIntMin(m_camera, L"FrameCount", &m_options.frameCountLimits[0]);
	AT_GetIntMax(m_camera, L"FrameCount", &m_options.frameCountLimits[1]);

	AT_GetIntMin(m_camera, L"AOIHeight", &m_options.ROIHeightLimits[0]);
	AT_GetIntMax(m_camera, L"AOIHeight", &m_options.ROIHeightLimits[1]);

	AT_GetIntMin(m_camera, L"AOIWidth", &m_options.ROIWidthLimits[0]);
	AT_GetIntMax(m_camera, L"AOIWidth", &m_options.ROIWidthLimits[1]);

	emit(optionsChanged(m_options));
}

void Andor::readSettings() {
	// general settings
	AT_GetFloat(m_camera, L"ExposureTime", &m_settings.exposureTime);
	//AT_GetInt(m_camera, L"FrameCount", &m_settings.frameCount);
	AT_GetBool(m_camera, L"SpuriousNoiseFilter", (int*)&m_settings.spuriousNoiseFilter);

	// ROI
	AT_GetInt(m_camera, L"AOIHeight", &m_settings.roi.height_binned);
	AT_GetInt(m_camera, L"AOIWidth", &m_settings.roi.width_binned);
	AT_GetInt(m_camera, L"AOILeft", &m_settings.roi.left);
	AT_GetInt(m_camera, L"AOITop", &m_settings.roi.top);
	getEnumString(L"AOIBinning", &m_settings.roi.binning);
	m_settings.roi.width_physical = m_settings.roi.width_binned * m_settings.roi.binX;
	m_settings.roi.height_physical = m_settings.roi.height_binned * m_settings.roi.binY;

	// readout parameters
	getEnumString(L"CycleMode", &m_settings.readout.cycleMode);
	getEnumString(L"Pixel Encoding", &m_settings.readout.pixelEncoding);
	getEnumString(L"Pixel Readout Rate", &m_settings.readout.pixelReadoutRate);
	getEnumString(L"SimplePreAmpGainControl", &m_settings.readout.preAmpGain);
	getEnumString(L"TriggerMode", &m_settings.readout.triggerMode);

	// emit signal that settings changed
	emit(settingsChanged(m_settings));
}

bool Andor::initialize() {
	if (!m_isInitialised) {
		auto i_retCode = AT_InitialiseLibrary();
		if (i_retCode != AT_SUCCESS) {
			//error condition, check atdebug.log file
			m_isInitialised = false;
		} else {
			// when AT_InitialiseLibrary is called when the camera is still disabled, it will succeed,
			// but the camera is never found even if switched on later
			auto i_numberOfDevices = AT_64{ 0 };
			// Use system handle as inidivdual handle to the camera hasn't been opened. 
			auto i_errorCode = AT_GetInt(AT_HANDLE_SYSTEM, L"DeviceCount", &i_numberOfDevices);
			if (i_numberOfDevices > 0) {
				m_isInitialised = true;
			} else {
				// if no camera is found and it was attempted to initialise the library, reinitializing will not help (wtf?)
				// the program has to be restarted
				emit(noCameraFound());
				AT_FinaliseLibrary();
				m_isInitialised = false;
			}
		}
	}
	return m_isInitialised;
}

void Andor::preparePreview() {
	// always use full camera image for live preview
	m_settings.roi.width_physical = m_options.ROIWidthLimits[1];
	m_settings.roi.left = 1;
	m_settings.roi.height_physical = m_options.ROIHeightLimits[1];
	m_settings.roi.top = 1;

	setSettings(m_settings);

	auto bufferSettings = BUFFER_SETTINGS{ 5, (unsigned int)m_settings.roi.bytesPerFrame, "unsigned short", m_settings.roi };
	m_previewBuffer->initializeBuffer(bufferSettings);
	emit(s_previewBufferSettingsChanged());

	// Start acquisition
	AT_Command(m_camera, L"AcquisitionStart");
	AT_InitialiseUtilityLibrary();
}

void Andor::cleanupAcquisition() {
	AT_FinaliseUtilityLibrary();
	AT_Command(m_camera, L"AcquisitionStop");
	AT_Flush(m_camera);
}

const std::string Andor::getTemperatureStatus() {
	auto i_retCode = AT_GetEnumIndex(m_camera, L"TemperatureStatus", &m_temperatureStatusIndex);
	AT_WC temperatureStatus[256];
	AT_GetEnumStringByIndex(m_camera, L"TemperatureStatus", m_temperatureStatusIndex, temperatureStatus, 256);
	std::wstring temperatureStatusString = temperatureStatus;
	m_temperatureStatus = std::string(temperatureStatusString.begin(), temperatureStatusString.end());
	return m_temperatureStatus;
}

double Andor::getSensorTemperature() {
	auto szValue{ 0.0 };
	auto i_retCode = AT_GetFloat(m_camera, L"SensorTemperature", &szValue);
	return szValue;
}

void Andor::getEnumString(AT_WC* feature, std::wstring* value) {
	auto enumIndex{ 0 };
	AT_GetEnumIndex(m_camera, feature, &enumIndex);
	AT_WC tmpValue[256];
	AT_GetEnumStringByIndex(m_camera, feature, enumIndex, tmpValue, 256);
	auto tmp = std::wstring(tmpValue);
	value = &tmp;
}

/*
 * Private slots
 */

void Andor::checkSensorTemperature() {
	m_sensorTemperature.temperature = getSensorTemperature();
	auto status = getTemperatureStatus();
	if (status == "Cooler Off") {
		m_sensorTemperature.status = enCameraTemperatureStatus::COOLER_OFF;
	} else if (status == "Fault") {
		m_sensorTemperature.status = enCameraTemperatureStatus::FAULT;
	} else if(status == "Cooling") {
		m_sensorTemperature.status = enCameraTemperatureStatus::COOLING;
	} else if (status == "Drift") {
		m_sensorTemperature.status = enCameraTemperatureStatus::DRIFT;
	} else if (status == "Not Stabilised") {
		m_sensorTemperature.status = enCameraTemperatureStatus::NOT_STABILISED;
	} else if (status == "Stabilised") {
		m_sensorTemperature.status = enCameraTemperatureStatus::STABILISED;
	} else {
		m_sensorTemperature.status = enCameraTemperatureStatus::FAULT;
	}
	emit(s_sensorTemperatureChanged(m_sensorTemperature));
}
