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
		m_isInitialised = TRUE;
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
			m_isConnected = TRUE;
		}
	}
}

void Andor::disconnect() {
	if (m_isConnected) {
		int i_retCode = AT_Close(m_cameraHndl);
		if (i_retCode == AT_SUCCESS) {
			m_isConnected = FALSE;
		}
	}
}

bool Andor::getConnectionStatus() {
	return m_isConnected;
}

void Andor::setSensorCooling(bool cooling) {
	int i_retCode = AT_SetBool(m_cameraHndl, L"SensorCooling", cooling);
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
		int ret = AT_WaitBuffer(m_cameraHndl, &Buffer, &m_bufferSize, 1500*m_exposureTime);

		// Process the image
		//Unpack the 12 bit packed data
		AT_GetInt(m_cameraHndl, L"AOI Height", &m_imageHeight);
		AT_GetInt(m_cameraHndl, L"AOI Width", &m_imageWidth);
		AT_GetInt(m_cameraHndl, L"AOI Stride", &m_imageStride);

		liveBuffer->m_freeBuffers->acquire();

		AT_ConvertBuffer(Buffer, liveBuffer->getWriteBuffer(), m_imageWidth, m_imageHeight, m_imageStride, L"Mono16", L"Mono16");

		liveBuffer->m_usedBuffers->release();

		delete[] Buffer;

		QMetaObject::invokeMethod(this, "acquire", Qt::QueuedConnection);
	} else {
		cleanupAcquisition();
	}
}

void Andor::prepareAcquisition() {
	// Set the pixel Encoding
	AT_SetEnumeratedString(m_cameraHndl, L"Pixel Encoding", m_pixelEncoding);

	// Set the pixel Readout Rate
	AT_SetEnumeratedString(m_cameraHndl, L"Pixel Readout Rate", m_pixelReadoutRate);

	// Set the exposure time
	AT_SetFloat(m_cameraHndl, L"ExposureTime", m_exposureTime);

	// enable spurious noise filter
	AT_SetBool(m_cameraHndl, L"SpuriousNoiseFilter", m_spuriousNoiseFilter);

	// Set the AOI
	AT_SetInt(m_cameraHndl, L"AOIWidth", m_imageWidth);
	AT_SetInt(m_cameraHndl, L"AOILeft", m_imageLeft);
	AT_SetInt(m_cameraHndl, L"AOIHeight", m_imageHeight);
	AT_SetInt(m_cameraHndl, L"AOITop", m_imageTop);
	AT_SetEnumeratedString(m_cameraHndl, L"AOIBinning", m_imageBinning);
	AT_SetEnumeratedString(m_cameraHndl, L"SimplePreAmpGainControl", m_preAmpGain);

	AT_SetEnumeratedString(m_cameraHndl, L"CycleMode", m_cycleMode);
	AT_SetEnumeratedString(m_cameraHndl, L"TriggerMode", m_triggerMode);

	// Allocate a buffer
	// Get the number of bytes required to store one frame
	AT_64 ImageSizeBytes;
	AT_GetInt(m_cameraHndl, L"ImageSizeBytes", &ImageSizeBytes);
	m_bufferSize = static_cast<int>(ImageSizeBytes);

	AT_GetInt(m_cameraHndl, L"AOIHeight", &m_imageHeight);
	AT_GetInt(m_cameraHndl, L"AOIWidth", &m_imageWidth);
	AT_GetInt(m_cameraHndl, L"AOILeft", &m_imageLeft);
	AT_GetInt(m_cameraHndl, L"AOITop", &m_imageTop);

	int pixelNumber = m_imageWidth * m_imageHeight;

	liveBuffer = new CircularBuffer<AT_U8>(5, pixelNumber * 2);

	// Start acquisition
	AT_Command(m_cameraHndl, L"AcquisitionStart");
	AT_InitialiseUtilityLibrary();

	emit(acquisitionRunning(TRUE, liveBuffer, m_imageWidth, m_imageHeight));
}

void Andor::cleanupAcquisition() {
	AT_FinaliseUtilityLibrary();
	AT_Command(m_cameraHndl, L"AcquisitionStop");
	AT_Flush(m_cameraHndl);
	emit(acquisitionRunning(FALSE, nullptr, 0, 0));
}