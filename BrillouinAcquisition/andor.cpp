#include "stdafx.h"
#include <iostream>
#include "andor.h"
#include "logger.h"
#include <windows.h>

Andor::Andor(QObject *parent)
	: QObject(parent) {
	int i_retCode;
	i_retCode = AT_InitialiseLibrary();
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
	int i_retCode;
	i_retCode = AT_GetEnumIndex(m_cameraHndl, L"TemperatureStatus", &m_temperatureStatusIndex);
	AT_GetEnumStringByIndex(m_cameraHndl, L"TemperatureStatus", m_temperatureStatusIndex, m_temperatureStatus, 256);
	return *m_temperatureStatus;
}

double Andor::getSensorTemperature() {
	double szValue;
	int i_retCode = AT_GetFloat(m_cameraHndl, L"SensorTemperature", &szValue);
	return szValue;
}

void Andor::acquireStartStop() {
// Set the camera settings

	//Set the pixel Encoding to the desired settings Mono16 Data
	AT_SetEnumeratedString(m_cameraHndl, L"Pixel Encoding", L"Mono16");

	//Set the pixel Readout Rate to slowest
	AT_SetEnumeratedString(m_cameraHndl, L"Pixel Readout Rate", L"100 MHz");

	//Set the exposure time for this camera to 10 milliseconds
	AT_SetFloat(m_cameraHndl, L"ExposureTime", 0.01);

	//Set the AOI
	AT_SetInt(m_cameraHndl, L"AOIWidth", 100);
	AT_SetInt(m_cameraHndl, L"AOILeft", 100);
	AT_SetInt(m_cameraHndl, L"AOIHeight", 100);
	AT_SetInt(m_cameraHndl, L"AOITop", 100);
	
// Allocate a buffer
	//Get the number of bytes required to store one frame
	AT_64 ImageSizeBytes;
	AT_GetInt(m_cameraHndl, L"ImageSizeBytes", &ImageSizeBytes);
	int BufferSize = static_cast<int>(ImageSizeBytes);
	
	//Allocate a memory buffer to store one frame
	unsigned char* UserBuffer = new unsigned char[BufferSize];
	
// Acquire images
	//Pass this buffer to the SDK
	AT_QueueBuffer(m_cameraHndl, UserBuffer, BufferSize);
	//Start the Acquisition running
	AT_Command(m_cameraHndl, L"AcquisitionStart");
	
	//Sleep in this thread until data is ready, in this case set
	//the timeout to infinite for simplicity
	unsigned char* Buffer;
	AT_WaitBuffer(m_cameraHndl, &Buffer, &BufferSize, AT_INFINITE);

// Process the image
	//Unpack the 12 bit packed data
	AT_InitialiseUtilityLibrary();
	AT_64 ImageHeight;
	AT_GetInt(m_cameraHndl, L"AOI Height", &ImageHeight);
	AT_64 ImageWidth;
	AT_GetInt(m_cameraHndl, L"AOI Width", &ImageWidth);
	AT_64 ImageStride;
	AT_GetInt(m_cameraHndl, L"AOI Stride", &ImageStride);
	
	// needed for Mono12Packed
	/*unsigned short* unpackedBuffer = new unsigned short[static_cast<size_t>(ImageHeight*ImageWidth)];
	AT_ConvertBuffer(Buffer, reinterpret_cast<unsigned char*>(unpackedBuffer), ImageWidth, ImageHeight, ImageStride, L"Mono12Packed", L"Mono16");*/


	int pixelNumber = ImageWidth*ImageHeight;
	AT_U8* ImagePixels = new AT_U8[pixelNumber*2];

	AT_ConvertBuffer(Buffer, ImagePixels, ImageWidth, ImageHeight, ImageStride, L"Mono16", L"Mono16");

	// Mono16
	unpackedBuffer = reinterpret_cast<unsigned short*>(ImagePixels);

	AT_FinaliseUtilityLibrary();

	//Stop the acquisition
	AT_Command(m_cameraHndl, L"AcquisitionStop");
	AT_Flush(m_cameraHndl);
	//Application specific data processing goes here..
	
	//Free the allocated buffer
	//delete [] UserBuffer;

	// announce image acquisition
	emit(imageAcquired(unpackedBuffer, ImageWidth, ImageHeight));
}

void Andor::acquireContinuously() {
	// Check if camera is currently acquiring images
	if (!m_isAcquiring) {
		m_isAcquiring = TRUE;
		// Set camera parameters

		// Set the pixel Encoding to the desired settings Mono16 Data
		AT_SetEnumeratedString(m_cameraHndl, L"Pixel Encoding", L"Mono16");

		// Set the pixel Readout Rate to slowest
		AT_SetEnumeratedString(m_cameraHndl, L"Pixel Readout Rate", L"100 MHz");

		// Set the exposure time for this camera to 10 milliseconds
		AT_SetFloat(m_cameraHndl, L"ExposureTime", 0.01);

		// Set the AOI
		AT_SetInt(m_cameraHndl, L"AOIWidth", 100);
		AT_SetInt(m_cameraHndl, L"AOILeft", 100);
		AT_SetInt(m_cameraHndl, L"AOIHeight", 100);
		AT_SetInt(m_cameraHndl, L"AOITop", 100);

		// Allocate a buffer
		// Get the number of bytes required to store one frame
		AT_64 ImageSizeBytes;
		AT_GetInt(m_cameraHndl, L"ImageSizeBytes", &ImageSizeBytes);
		BufferSize = static_cast<int>(ImageSizeBytes);

		//Allocate a memory buffer to store one frame
		UserBuffer = new unsigned char[BufferSize];

		// Acquire images
		// Pass this buffer to the SDK
		AT_QueueBuffer(m_cameraHndl, UserBuffer, BufferSize);

		// Start acquisition
		AT_Command(m_cameraHndl, L"AcquisitionStart");

		// Sleep in this thread until data is ready, in this case set
		// the timeout to infinite for simplicity
		AT_WaitBuffer(m_cameraHndl, &Buffer, &BufferSize, AT_INFINITE);
		AT_InitialiseUtilityLibrary();

		acquire();
		//QMetaObject::invokeMethod(this, "acquire", Qt::QueuedConnection);
	} else {
		m_isAcquiring = FALSE;
		delete UserBuffer;
		// Stop acquisition
		AT_FinaliseUtilityLibrary();
		AT_Command(m_cameraHndl, L"AcquisitionStop");
		AT_Flush(m_cameraHndl);
	}
}

void Andor::acquire() {
	if (m_isAcquiring) {
		// Acquire camera images

		// Process the image
		//Unpack the 12 bit packed data
		AT_64 ImageHeight;
		AT_GetInt(m_cameraHndl, L"AOI Height", &ImageHeight);
		AT_64 ImageWidth;
		AT_GetInt(m_cameraHndl, L"AOI Width", &ImageWidth);
		AT_64 ImageStride;
		AT_GetInt(m_cameraHndl, L"AOI Stride", &ImageStride);

		// needed for Mono12Packed
		/*unsigned short* unpackedBuffer = new unsigned short[static_cast<size_t>(ImageHeight*ImageWidth)];
		AT_ConvertBuffer(Buffer, reinterpret_cast<unsigned char*>(unpackedBuffer), ImageWidth, ImageHeight, ImageStride, L"Mono12Packed", L"Mono16");*/


		int pixelNumber = ImageWidth * ImageHeight;
		AT_U8* ImagePixels = new AT_U8[pixelNumber * 2];

		AT_ConvertBuffer(Buffer, ImagePixels, ImageWidth, ImageHeight, ImageStride, L"Mono16", L"Mono16");

		// Mono16
		unpackedBuffer = reinterpret_cast<unsigned short*>(ImagePixels);

		// announce image acquisition
		emit(imageAcquired(unpackedBuffer, ImageWidth, ImageHeight));

		Sleep(500);

		delete ImagePixels;

		QMetaObject::invokeMethod(this, "acquire", Qt::QueuedConnection);
	}
}

/*
 *	Currently only a test function for asynchronous execution
 */
void Andor::acquireSingleTest(int index, std::string test) {
	qInfo(logInfo()) << "Acquisition started.";
	Sleep(5000);
	qInfo(logInfo()) << "Acquisition finished.";
}

/*
*	Currently only a test function for asynchronous execution
*/
void Andor::acquireSingle() {
	qInfo(logInfo()) << "Acquisition started.";
	Sleep(5000);
	qInfo(logInfo()) << "Acquisition finished.";
}