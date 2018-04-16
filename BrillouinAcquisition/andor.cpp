#include "stdafx.h"
#include <iostream>
#include "andor.h"
#include "logger.h"
#include <windows.h>

Andor::Andor(QObject *parent)
	: QObject(parent) {
	int i_retCode = AT_InitialiseLibrary();
	if (i_retCode != AT_SUCCESS) {
		//error condition, check atdebug.log file
	} else {
		m_isInitialised = true;
	}
}

Andor::~Andor() {
	if (m_isConnected) {
		AT_Close(m_cameraHndl);
	}
	AT_FinaliseLibrary();
}

void Andor::connect() {
	if (!m_isConnected) {
		int i_retCode = AT_Open(0, &m_cameraHndl);
		if (i_retCode == AT_SUCCESS) {
			m_isConnected = true;
			readOptions();
			setDefaultSettings();
			readSettings();
		}
	}
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
	AT_SetEnumeratedString(m_cameraHndl, L"AOIBinning", m_settings.roi.binning);

	// readout parameters
	AT_SetEnumeratedString(m_cameraHndl, L"CycleMode", m_settings.readout.cycleMode);
	AT_SetEnumeratedString(m_cameraHndl, L"Pixel Encoding", m_settings.readout.pixelEncoding);
	AT_SetEnumeratedString(m_cameraHndl, L"Pixel Readout Rate", m_settings.readout.pixelReadoutRate);
	AT_SetEnumeratedString(m_cameraHndl, L"SimplePreAmpGainControl", m_settings.readout.preAmpGain);
	AT_SetEnumeratedString(m_cameraHndl, L"TriggerMode", m_settings.readout.triggerMode);
};

void Andor::readSettings() {
	// general settings
	AT_GetFloat(m_cameraHndl, L"ExposureTime", &m_settings.exposureTime);
	AT_GetInt(m_cameraHndl, L"FrameCount", &m_settings.frameCount);
	AT_GetBool(m_cameraHndl, L"SpuriousNoiseFilter", &m_settings.spuriousNoiseFilter);

	// ROI
	AT_GetInt(m_cameraHndl, L"AOIHeight", &m_settings.roi.height);
	AT_GetInt(m_cameraHndl, L"AOIWidth", &m_settings.roi.width);
	AT_GetInt(m_cameraHndl, L"AOILeft", &m_settings.roi.left);
	AT_GetInt(m_cameraHndl, L"AOITop", &m_settings.roi.top);
	getEnumString(L"AOIBinning", m_settings.roi.binning);

	// readout parameters
	getEnumString(L"CycleMode", m_settings.readout.cycleMode);
	getEnumString(L"Pixel Encoding", m_settings.readout.pixelEncoding);
	getEnumString(L"Pixel Readout Rate", m_settings.readout.pixelReadoutRate);
	getEnumString(L"SimplePreAmpGainControl", m_settings.readout.preAmpGain);
	getEnumString(L"TriggerMode", m_settings.readout.triggerMode);

	// emit signal that settings changed
	emit(settingsChanged(m_settings));
};

void Andor::disconnect() {
	if (m_isConnected) {
		int i_retCode = AT_Close(m_cameraHndl);
		if (i_retCode == AT_SUCCESS) {
			m_isConnected = false;
		}
	}
}

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
}

bool Andor::getSensorCooling() {
	AT_BOOL szValue;
	int i_retCode = AT_GetBool(m_cameraHndl, L"SensorCooling", &szValue);
	return szValue;
}

const wchar_t Andor::getTemperatureStatus() {
	int i_retCode = AT_GetEnumIndex(m_cameraHndl, L"TemperatureStatus", &m_temperatureStatusIndex);
	AT_GetEnumStringByIndex(m_cameraHndl, L"TemperatureStatus", m_temperatureStatusIndex, m_temperatureStatus, 256);
	return *m_temperatureStatus;
}

double Andor::getSensorTemperature() {
	double szValue;
	int i_retCode = AT_GetFloat(m_cameraHndl, L"SensorTemperature", &szValue);
	return szValue;
}

void Andor::acquireContinuously() {
	// Check if camera is currently acquiring images
	m_isAcquiring = !m_isAcquiring;
	if (m_isAcquiring) {
		prepareAcquisition();
		acquire();
	} else {
		cleanupAcquisition();
	}
}

void Andor::acquire() {
	if (m_isAcquiring) {

		// Pass this buffer to the SDK
		unsigned char* UserBuffer = new unsigned char[m_bufferSize];
		AT_QueueBuffer(m_cameraHndl, UserBuffer, m_bufferSize);

		// Acquire camera images
		AT_Command(m_cameraHndl, L"SoftwareTrigger");

		// Sleep in this thread until data is ready
		unsigned char* Buffer;
		int ret = AT_WaitBuffer(m_cameraHndl, &Buffer, &m_bufferSize, 1500*m_settings.exposureTime);

		// Process the image
		//Unpack the 12 bit packed data
		AT_GetInt(m_cameraHndl, L"AOIHeight", &m_settings.roi.height);
		AT_GetInt(m_cameraHndl, L"AOIWidth", &m_settings.roi.width);
		AT_GetInt(m_cameraHndl, L"AOIStride", &m_imageStride);

		liveBuffer->m_freeBuffers->acquire();

		AT_ConvertBuffer(Buffer, liveBuffer->getWriteBuffer(), m_settings.roi.width, m_settings.roi.height, m_imageStride, m_settings.readout.pixelEncoding, L"Mono16");

		liveBuffer->m_usedBuffers->release();

		delete[] Buffer;

		QMetaObject::invokeMethod(this, "acquire", Qt::QueuedConnection);
	} else {
		cleanupAcquisition();
	}
}

void Andor::setSettings() {
	// Set the pixel Encoding
	AT_SetEnumeratedString(m_cameraHndl, L"Pixel Encoding", m_settings.readout.pixelEncoding);

	// Set the pixel Readout Rate
	AT_SetEnumeratedString(m_cameraHndl, L"Pixel Readout Rate", m_settings.readout.pixelReadoutRate);

	// Set the exposure time
	AT_SetFloat(m_cameraHndl, L"ExposureTime", m_settings.exposureTime);

	// enable spurious noise filter
	AT_SetBool(m_cameraHndl, L"SpuriousNoiseFilter", m_settings.spuriousNoiseFilter);

	// Set the AOI
	AT_SetInt(m_cameraHndl, L"AOIWidth", m_settings.roi.width);
	AT_SetInt(m_cameraHndl, L"AOILeft", m_settings.roi.left);
	AT_SetInt(m_cameraHndl, L"AOIHeight", m_settings.roi.height);
	AT_SetInt(m_cameraHndl, L"AOITop", m_settings.roi.top);
	AT_SetEnumeratedString(m_cameraHndl, L"AOIBinning", m_settings.roi.binning);
	AT_SetEnumeratedString(m_cameraHndl, L"SimplePreAmpGainControl", m_settings.readout.preAmpGain);

	AT_SetEnumeratedString(m_cameraHndl, L"CycleMode", m_settings.readout.cycleMode);
	AT_SetEnumeratedString(m_cameraHndl, L"TriggerMode", m_settings.readout.triggerMode);

	// Allocate a buffer
	// Get the number of bytes required to store one frame
	AT_64 ImageSizeBytes;
	AT_GetInt(m_cameraHndl, L"ImageSizeBytes", &ImageSizeBytes);
	m_bufferSize = static_cast<int>(ImageSizeBytes);

	AT_GetInt(m_cameraHndl, L"AOIHeight", &m_settings.roi.height);
	AT_GetInt(m_cameraHndl, L"AOIWidth", &m_settings.roi.width);
	AT_GetInt(m_cameraHndl, L"AOILeft", &m_settings.roi.left);
	AT_GetInt(m_cameraHndl, L"AOITop", &m_settings.roi.top);
}

void Andor::prepareAcquisition() {
	setSettings();

	int pixelNumber = m_settings.roi.width * m_settings.roi.height;
	liveBuffer = new CircularBuffer<AT_U8>(5, pixelNumber * 2);

	AT_SetInt(m_cameraHndl, L"AOIWidth", m_options.ROIWidthLimits[1]);
	AT_SetInt(m_cameraHndl, L"AOILeft", m_options.ROIWidthLimits[0]);
	AT_SetInt(m_cameraHndl, L"AOIHeight", m_options.ROIHeightLimits[1]);
	AT_SetInt(m_cameraHndl, L"AOITop", m_options.ROIHeightLimits[0]);

	AT_GetInt(m_cameraHndl, L"AOIHeight", &m_settings.roi.height);
	AT_GetInt(m_cameraHndl, L"AOIWidth", &m_settings.roi.width);
	AT_GetInt(m_cameraHndl, L"AOILeft", &m_settings.roi.left);
	AT_GetInt(m_cameraHndl, L"AOITop", &m_settings.roi.top);

	// Start acquisition
	AT_Command(m_cameraHndl, L"AcquisitionStart");
	AT_InitialiseUtilityLibrary();

	emit(s_previewRunning(true));
	emit(acquisitionRunning(true, liveBuffer, m_settings.roi.width, m_settings.roi.height));
}

void Andor::cleanupAcquisition() {
	AT_FinaliseUtilityLibrary();
	AT_Command(m_cameraHndl, L"AcquisitionStop");
	AT_Flush(m_cameraHndl);
	emit(s_previewRunning(false));
	emit(acquisitionRunning(false, nullptr, 0, 0));
}


void Andor::prepareMeasurement(CAMERA_SETTINGS settings) {
	m_settings = settings;
	setSettings();

	// Start acquisition
	AT_Command(m_cameraHndl, L"AcquisitionStart");
	AT_InitialiseUtilityLibrary();
};

void Andor::acquireImage(AT_U8* buffer) {
	// Pass this buffer to the SDK
	unsigned char* UserBuffer = new unsigned char[m_bufferSize];
	AT_QueueBuffer(m_cameraHndl, UserBuffer, m_bufferSize);

	// Acquire camera images
	AT_Command(m_cameraHndl, L"SoftwareTrigger");

	// Sleep in this thread until data is ready
	unsigned char* Buffer;
	int ret = AT_WaitBuffer(m_cameraHndl, &Buffer, &m_bufferSize, 1500 * m_settings.exposureTime);

	// Process the image
	//Unpack the 12 bit packed data
	AT_GetInt(m_cameraHndl, L"AOIHeight", &m_settings.roi.height);
	AT_GetInt(m_cameraHndl, L"AOIWidth", &m_settings.roi.width);
	AT_GetInt(m_cameraHndl, L"AOIStride", &m_imageStride);

	AT_ConvertBuffer(Buffer, buffer, m_settings.roi.width, m_settings.roi.height, m_imageStride, m_settings.readout.pixelEncoding, L"Mono16");

	delete[] Buffer;
};

void Andor::setCalibrationExposureTime(double exposureTime) {
	m_settings.exposureTime = exposureTime;
	AT_Command(m_cameraHndl, L"AcquisitionStop");
	// Set the exposure time
	AT_SetFloat(m_cameraHndl, L"ExposureTime", m_settings.exposureTime);

	AT_Command(m_cameraHndl, L"AcquisitionStart");
}