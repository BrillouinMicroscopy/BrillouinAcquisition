#include "stdafx.h"
#include <iostream>
#include "andor.h"
#include "logger.h"
#include <windows.h>

Andor::Andor(QObject *parent) noexcept
	: QObject(parent) {
}

Andor::~Andor() {
	if (m_isConnected) {
		AT_Close(m_cameraHndl);
	}
	AT_FinaliseLibrary();
}

bool Andor::initialize() {
	if (!m_isInitialised) {
		int i_retCode = AT_InitialiseLibrary();
		if (i_retCode != AT_SUCCESS) {
			//error condition, check atdebug.log file
			m_isInitialised = false;
		} else {
			// when AT_InitialiseLibrary is called when the camera is still disabled, it will succeed,
			// but the camera is never found even if switched on later
			AT_64 i_numberOfDevices = 0;
			// Use system handle as inidivdual handle to the camera hasn't been opened. 
			int i_errorCode = AT_GetInt(AT_HANDLE_SYSTEM, L"DeviceCount", &i_numberOfDevices);
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

void Andor::init() {
	// create timers and connect their signals
	// after moving andor to another thread
	m_tempTimer = new QTimer();
	QMetaObject::Connection connection = QWidget::connect(m_tempTimer, SIGNAL(timeout()), this, SLOT(checkSensorTemperature()));
}

void Andor::connectDevice() {
	// initialize library
	initialize();
	if (!m_isConnected && m_isInitialised) {
		int i_retCode = AT_Open(0, &m_cameraHndl);
		if (i_retCode == AT_SUCCESS) {
			m_isConnected = true;
			readOptions();
			setDefaultSettings();
			readSettings();
			if (!m_tempTimer->isActive()) {
				m_tempTimer->start(1000);
			}
		}
	}
	emit(cameraConnected(m_isConnected));
}

void Andor::disconnectDevice() {
	if (m_isConnected) {
		if (m_tempTimer->isActive()) {
			m_tempTimer->stop();
		}
		int i_retCode = AT_Close(m_cameraHndl);
		if (i_retCode == AT_SUCCESS) {
			m_isConnected = false;
		}
	}
	emit(cameraConnected(m_isConnected));
}

void Andor::readOptions() {

	AT_GetFloatMin(m_cameraHndl, L"ExposureTime", &m_options.exposureTimeLimits[0]);
	AT_GetFloatMax(m_cameraHndl, L"ExposureTime", &m_options.exposureTimeLimits[1]);
	AT_GetIntMin(m_cameraHndl, L"FrameCount", &m_options.frameCountLimits[0]);
	AT_GetIntMax(m_cameraHndl, L"FrameCount", &m_options.frameCountLimits[1]);

	AT_GetIntMin(m_cameraHndl, L"AOIHeight", &m_options.ROIHeightLimits[0]);
	AT_GetIntMax(m_cameraHndl, L"AOIHeight", &m_options.ROIHeightLimits[1]);

	AT_GetIntMin(m_cameraHndl, L"AOIWidth", &m_options.ROIWidthLimits[0]);
	AT_GetIntMax(m_cameraHndl, L"AOIWidth", &m_options.ROIWidthLimits[1]);

	emit(optionsChanged(m_options));
}

void Andor::setDefaultSettings() {
	// general settings
	AT_SetFloat(m_cameraHndl, L"ExposureTime", m_settings.exposureTime);
	AT_SetInt(m_cameraHndl, L"FrameCount", m_settings.frameCount);
	AT_SetBool(m_cameraHndl, L"SpuriousNoiseFilter", m_settings.spuriousNoiseFilter);

	// ROI
	AT_SetInt(m_cameraHndl, L"AOIHeight", m_settings.roi.height);
	AT_SetInt(m_cameraHndl, L"AOIWidth", m_settings.roi.width);
	AT_SetInt(m_cameraHndl, L"AOILeft", m_settings.roi.left);
	AT_SetInt(m_cameraHndl, L"AOITop", m_settings.roi.top);
	AT_SetEnumeratedString(m_cameraHndl, L"AOIBinning", m_settings.roi.binning.c_str());

	// readout parameters
	AT_SetEnumeratedString(m_cameraHndl, L"CycleMode", m_settings.readout.cycleMode.c_str());
	AT_SetEnumeratedString(m_cameraHndl, L"Pixel Encoding", m_settings.readout.pixelEncoding.c_str());
	AT_SetEnumeratedString(m_cameraHndl, L"Pixel Readout Rate", m_settings.readout.pixelReadoutRate.c_str());
	AT_SetEnumeratedString(m_cameraHndl, L"SimplePreAmpGainControl", m_settings.readout.preAmpGain.c_str());
	AT_SetEnumeratedString(m_cameraHndl, L"TriggerMode", m_settings.readout.triggerMode.c_str());
};

CAMERA_SETTINGS Andor::readSettings() {
	// general settings
	AT_GetFloat(m_cameraHndl, L"ExposureTime", &m_settings.exposureTime);
	AT_GetInt(m_cameraHndl, L"FrameCount", &m_settings.frameCount);
	AT_GetBool(m_cameraHndl, L"SpuriousNoiseFilter", &m_settings.spuriousNoiseFilter);

	// ROI
	AT_GetInt(m_cameraHndl, L"AOIHeight", &m_settings.roi.height);
	AT_GetInt(m_cameraHndl, L"AOIWidth", &m_settings.roi.width);
	AT_GetInt(m_cameraHndl, L"AOILeft", &m_settings.roi.left);
	AT_GetInt(m_cameraHndl, L"AOITop", &m_settings.roi.top);
	getEnumString(L"AOIBinning", &m_settings.roi.binning[0]);

	// readout parameters
	getEnumString(L"CycleMode", &m_settings.readout.cycleMode[0]);
	getEnumString(L"Pixel Encoding", &m_settings.readout.pixelEncoding[0]);
	getEnumString(L"Pixel Readout Rate", &m_settings.readout.pixelReadoutRate[0]);
	getEnumString(L"SimplePreAmpGainControl", &m_settings.readout.preAmpGain[0]);
	getEnumString(L"TriggerMode", &m_settings.readout.triggerMode[0]);

	// emit signal that settings changed
	emit(settingsChanged(m_settings));

	return m_settings;
};

void Andor::getEnumString(AT_WC* feature, AT_WC* string) {
	int enumIndex;
	AT_GetEnumIndex(m_cameraHndl, feature, &enumIndex);
	AT_GetEnumStringByIndex(m_cameraHndl, feature, enumIndex, string, 256);
}

bool Andor::getConnectionStatus() {
	return m_isConnected;
}

void Andor::setSensorCooling(bool cooling) {
	int i_retCode = AT_SetBool(m_cameraHndl, L"SensorCooling", (int)cooling);
	m_isCooling = cooling;
	emit(cameraCoolingChanged(m_isCooling));
}

bool Andor::getSensorCooling() {
	AT_BOOL szValue;
	int i_retCode = AT_GetBool(m_cameraHndl, L"SensorCooling", &szValue);
	return szValue;
}

const std::string Andor::getTemperatureStatus() {
	int i_retCode = AT_GetEnumIndex(m_cameraHndl, L"TemperatureStatus", &m_temperatureStatusIndex);
	AT_GetEnumStringByIndex(m_cameraHndl, L"TemperatureStatus", m_temperatureStatusIndex, temperatureStatus, 256);
	std::wstring ws(temperatureStatus);
	std::string m_temperatureStatus(ws.begin(), ws.end());
	return m_temperatureStatus;
}

double Andor::getSensorTemperature() {
	double szValue;
	int i_retCode = AT_GetFloat(m_cameraHndl, L"SensorTemperature", &szValue);
	return szValue;
}

void Andor::checkSensorTemperature() {
	m_sensorTemperature.temperature = getSensorTemperature();
	std::string status = getTemperatureStatus();
	if (status == "Cooler Off") {
		m_sensorTemperature.status = COOLER_OFF;
	} else if (status == "Fault") {
		m_sensorTemperature.status = FAULT;
	} else if(status == "Cooling") {
		m_sensorTemperature.status = COOLING;
	} else if (status == "Drift") {
		m_sensorTemperature.status = DRIFT;
	} else if (status == "Not Stabilised") {
		m_sensorTemperature.status = NOT_STABILISED;
	} else if (status == "Stabilised") {
		m_sensorTemperature.status = STABILISED;
	} else {
		m_sensorTemperature.status = FAULT;
	}
	emit(s_sensorTemperatureChanged(m_sensorTemperature));
}

void Andor::startPreview(CAMERA_SETTINGS settings) {
	m_isPreviewRunning = true;
	m_settings = settings;
	preparePreview();
	getImageForPreview();
}

void Andor::preparePreview() {
	// always use full camera image for live preview
	m_settings.roi.width = m_options.ROIWidthLimits[1];
	m_settings.roi.left = 1;
	m_settings.roi.height = m_options.ROIHeightLimits[1];
	m_settings.roi.top = 1;

	setSettings();

	int pixelNumber = m_settings.roi.width * m_settings.roi.height;
	BUFFER_SETTINGS bufferSettings = { 5, pixelNumber * 2, m_settings.roi };
	previewBuffer->initializeBuffer(bufferSettings);
	emit(s_previewBufferSettingsChanged());

	// Start acquisition
	AT_Command(m_cameraHndl, L"AcquisitionStart");
	AT_InitialiseUtilityLibrary();

	emit(s_previewRunning(true));
}

void Andor::stopPreview() {
	m_isPreviewRunning = false;
	cleanupAcquisition();
	emit(s_previewRunning(false));
}

CAMERA_SETTINGS Andor::prepareMeasurement(CAMERA_SETTINGS settings) {
	m_settings = settings;

	// check if currently a preview is running and stop it in case
	if (m_isPreviewRunning) {
		stopPreview();
	}

	setSettings();

	int pixelNumber = m_settings.roi.width * m_settings.roi.height;
	BUFFER_SETTINGS bufferSettings = { 4, pixelNumber * 2, m_settings.roi };
	previewBuffer->initializeBuffer(bufferSettings);
	emit(s_previewBufferSettingsChanged());

	// Start acquisition
	AT_Command(m_cameraHndl, L"AcquisitionStart");
	AT_InitialiseUtilityLibrary();

	emit(s_measurementRunning(true));

	return readSettings();
};

void Andor::stopMeasurement() {
	cleanupAcquisition();
	emit(s_measurementRunning(false));
}

void Andor::cleanupAcquisition() {
	AT_FinaliseUtilityLibrary();
	AT_Command(m_cameraHndl, L"AcquisitionStop");
	AT_Flush(m_cameraHndl);
}

void Andor::acquireImage(AT_U8* buffer) {
	// Pass this buffer to the SDK
	unsigned char* UserBuffer = new unsigned char[m_bufferSize];
	AT_QueueBuffer(m_cameraHndl, UserBuffer, m_bufferSize);

	// Acquire camera images
	AT_Command(m_cameraHndl, L"SoftwareTrigger");

	// Sleep in this thread until data is ready
	unsigned char* Buffer;
	int ret = AT_WaitBuffer(m_cameraHndl, &Buffer, &m_bufferSize, 1500 * m_settings.exposureTime);
	// return if AT_WaitBuffer timed out
	if (ret == AT_ERR_TIMEDOUT) {
		return;
	}

	// Process the image
	//Unpack the 12 bit packed data
	AT_GetInt(m_cameraHndl, L"AOIHeight", &m_settings.roi.height);
	AT_GetInt(m_cameraHndl, L"AOIWidth", &m_settings.roi.width);
	AT_GetInt(m_cameraHndl, L"AOIStride", &m_imageStride);

	AT_ConvertBuffer(Buffer, buffer, m_settings.roi.width, m_settings.roi.height, m_imageStride, m_settings.readout.pixelEncoding.c_str(), L"Mono16");

	delete[] Buffer;
};

void Andor::getImageForPreview() {
	if (m_isPreviewRunning) {

		previewBuffer->m_buffer->m_freeBuffers->acquire();
		acquireImage(previewBuffer->m_buffer->getWriteBuffer());
		previewBuffer->m_buffer->m_usedBuffers->release();

		QMetaObject::invokeMethod(this, "getImageForPreview", Qt::QueuedConnection);
	} else {
		stopPreview();
	}
}

void Andor::getImageForMeasurement(AT_U8* buffer) {
	acquireImage(buffer);

	// write image to preview buffer
	memcpy(previewBuffer->m_buffer->getWriteBuffer(), buffer, m_settings.roi.width * m_settings.roi.height * 2);
	previewBuffer->m_buffer->m_usedBuffers->release();
}

void Andor::setSettings() {
	// Set the pixel Encoding
	AT_SetEnumeratedString(m_cameraHndl, L"Pixel Encoding", m_settings.readout.pixelEncoding.c_str());

	// Set the pixel Readout Rate
	AT_SetEnumeratedString(m_cameraHndl, L"Pixel Readout Rate", m_settings.readout.pixelReadoutRate.c_str());

	// Set the exposure time
	AT_SetFloat(m_cameraHndl, L"ExposureTime", m_settings.exposureTime);

	// enable spurious noise filter
	AT_SetBool(m_cameraHndl, L"SpuriousNoiseFilter", m_settings.spuriousNoiseFilter);

	// Set the AOI
	AT_SetInt(m_cameraHndl, L"AOIWidth", m_settings.roi.width);
	AT_SetInt(m_cameraHndl, L"AOILeft", m_settings.roi.left);
	AT_SetInt(m_cameraHndl, L"AOIHeight", m_settings.roi.height);
	AT_SetInt(m_cameraHndl, L"AOITop", m_settings.roi.top);
	AT_SetEnumeratedString(m_cameraHndl, L"AOIBinning", m_settings.roi.binning.c_str());
	AT_SetEnumeratedString(m_cameraHndl, L"SimplePreAmpGainControl", m_settings.readout.preAmpGain.c_str());

	AT_SetEnumeratedString(m_cameraHndl, L"CycleMode", m_settings.readout.cycleMode.c_str());
	AT_SetEnumeratedString(m_cameraHndl, L"TriggerMode", m_settings.readout.triggerMode.c_str());

	// Allocate a buffer
	// Get the number of bytes required to store one frame
	AT_64 ImageSizeBytes;
	AT_GetInt(m_cameraHndl, L"ImageSizeBytes", &ImageSizeBytes);
	m_bufferSize = static_cast<int>(ImageSizeBytes);

	AT_GetInt(m_cameraHndl, L"AOIHeight", &m_settings.roi.height);
	AT_GetInt(m_cameraHndl, L"AOIWidth", &m_settings.roi.width);
	AT_GetInt(m_cameraHndl, L"AOILeft", &m_settings.roi.left);
	AT_GetInt(m_cameraHndl, L"AOITop", &m_settings.roi.top);

	// emit signal that settings changed
	emit(settingsChanged(m_settings));
}

void Andor::setCalibrationExposureTime(double exposureTime) {
	m_settings.exposureTime = exposureTime;
	AT_Command(m_cameraHndl, L"AcquisitionStop");
	// Set the exposure time
	AT_SetFloat(m_cameraHndl, L"ExposureTime", m_settings.exposureTime);

	AT_Command(m_cameraHndl, L"AcquisitionStart");
}