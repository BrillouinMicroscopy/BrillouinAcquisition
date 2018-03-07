#include "stdafx.h"
#include <iostream>
#include "andor.h"
#include "logger.h"
#include <windows.h>

Andor::Andor(QObject *parent)
	: QObject(parent) {
	m_abort = false;
	int i_retCode;
	i_retCode = AT_InitialiseLibrary();
	if (i_retCode != AT_SUCCESS) {
		//error condition, check atdebug.log file
	} else {
		initialised = TRUE;
	}
}

Andor::~Andor() {
	if (connected) {
		AT_Close(Hndl);
	}
	AT_FinaliseLibrary();
}

void Andor::connect() {
	if (!connected) {
		int i_retCode = AT_Open(0, &Hndl);
		if (i_retCode == AT_SUCCESS) {
			connected = TRUE;
		}
	}
}

void Andor::disconnect() {
	if (connected) {
		int i_retCode = AT_Close(Hndl);
		if (i_retCode == AT_SUCCESS) {
			connected = FALSE;
		}
	}
}

bool Andor::getConnectionStatus() {
	return connected;
}

void Andor::setSensorCooling(bool cooling) {
	int i_retCode = AT_SetBool(Hndl, L"SensorCooling", cooling);
}

bool Andor::getSensorCooling() {
	AT_BOOL szValue;
	int i_retCode = AT_GetBool(Hndl, L"SensorCooling", &szValue);
	return szValue;
}

const wchar_t Andor::getTemperatureStatus() {
	int i_retCode;
	i_retCode = AT_GetEnumIndex(Hndl, L"TemperatureStatus", &temperatureStatusIndex);
	AT_GetEnumStringByIndex(Hndl, L"TemperatureStatus", temperatureStatusIndex, temperatureStatus, 256);
	return *temperatureStatus;
}

double Andor::getSensorTemperature() {
	double szValue;
	int i_retCode = AT_GetFloat(Hndl, L"SensorTemperature", &szValue);
	return szValue;
}

void Andor::acquireStartStop() {
// Set the camera settings

	//Set the pixel Encoding to the desired settings Mono16 Data
	AT_SetEnumeratedString(Hndl, L"Pixel Encoding", L"Mono16");

	//Set the pixel Readout Rate to slowest
	AT_SetEnumeratedString(Hndl, L"Pixel Readout Rate", L"100 MHz");

	//Set the exposure time for this camera to 10 milliseconds
	AT_SetFloat(Hndl, L"ExposureTime", 0.01);

	//Set the AOI
	AT_SetInt(Hndl, L"AOIWidth", 100);
	AT_SetInt(Hndl, L"AOILeft", 100);
	AT_SetInt(Hndl, L"AOIHeight", 100);
	AT_SetInt(Hndl, L"AOITop", 100);
	
// Allocate a buffer
	//Get the number of bytes required to store one frame
	AT_64 ImageSizeBytes;
	AT_GetInt(Hndl, L"ImageSizeBytes", &ImageSizeBytes);
	int BufferSize = static_cast<int>(ImageSizeBytes);
	
	//Allocate a memory buffer to store one frame
	unsigned char* UserBuffer = new unsigned char[BufferSize];
	
// Acquire images
	//Pass this buffer to the SDK
	AT_QueueBuffer(Hndl, UserBuffer, BufferSize);
	//Start the Acquisition running
	AT_Command(Hndl, L"AcquisitionStart");
	
	//Sleep in this thread until data is ready, in this case set
	//the timeout to infinite for simplicity
	unsigned char* Buffer;
	AT_WaitBuffer(Hndl, &Buffer, &BufferSize, AT_INFINITE);

// Process the image
	//Unpack the 12 bit packed data
	AT_InitialiseUtilityLibrary();
	AT_64 ImageHeight;
	AT_GetInt(Hndl, L"AOI Height", &ImageHeight);
	AT_64 ImageWidth;
	AT_GetInt(Hndl, L"AOI Width", &ImageWidth);
	AT_64 ImageStride;
	AT_GetInt(Hndl, L"AOI Stride", &ImageStride);
	
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
	AT_Command(Hndl, L"AcquisitionStop");
	AT_Flush(Hndl);
	//Application specific data processing goes here..
	
	//Free the allocated buffer
	//delete [] UserBuffer;

	// announce image acquisition
	emit(imageAcquired(unpackedBuffer, ImageWidth, ImageHeight));
}

void Andor::acquireContinuously() {
	// Check if camera is currently acquiring images
	if (!m_acquiring) {
		m_acquiring = TRUE;
		// Set camera parameters

		// Set the pixel Encoding to the desired settings Mono16 Data
		AT_SetEnumeratedString(Hndl, L"Pixel Encoding", L"Mono16");

		// Set the pixel Readout Rate to slowest
		AT_SetEnumeratedString(Hndl, L"Pixel Readout Rate", L"100 MHz");

		// Set the exposure time for this camera to 10 milliseconds
		AT_SetFloat(Hndl, L"ExposureTime", 0.01);

		// Set the AOI
		AT_SetInt(Hndl, L"AOIWidth", 100);
		AT_SetInt(Hndl, L"AOILeft", 100);
		AT_SetInt(Hndl, L"AOIHeight", 100);
		AT_SetInt(Hndl, L"AOITop", 100);

		// Allocate a buffer
		// Get the number of bytes required to store one frame
		AT_64 ImageSizeBytes;
		AT_GetInt(Hndl, L"ImageSizeBytes", &ImageSizeBytes);
		BufferSize = static_cast<int>(ImageSizeBytes);

		//Allocate a memory buffer to store one frame
		UserBuffer = new unsigned char[BufferSize];

		// Acquire images
		// Pass this buffer to the SDK
		AT_QueueBuffer(Hndl, UserBuffer, BufferSize);

		// Start acquisition
		AT_Command(Hndl, L"AcquisitionStart");

		// Sleep in this thread until data is ready, in this case set
		// the timeout to infinite for simplicity
		AT_WaitBuffer(Hndl, &Buffer, &BufferSize, AT_INFINITE);
		AT_InitialiseUtilityLibrary();

		acquire();
		//QMetaObject::invokeMethod(this, "acquire", Qt::QueuedConnection);
	} else {
		m_acquiring = FALSE;
		delete UserBuffer;
		// Stop acquisition
		AT_FinaliseUtilityLibrary();
		AT_Command(Hndl, L"AcquisitionStop");
		AT_Flush(Hndl);
	}
}

void Andor::acquire() {
	if (m_acquiring) {
		// Acquire camera images

		// Process the image
		//Unpack the 12 bit packed data
		AT_64 ImageHeight;
		AT_GetInt(Hndl, L"AOI Height", &ImageHeight);
		AT_64 ImageWidth;
		AT_GetInt(Hndl, L"AOI Width", &ImageWidth);
		AT_64 ImageStride;
		AT_GetInt(Hndl, L"AOI Stride", &ImageStride);

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

void Andor::checkCamera() {
	qInfo(logInfo()) << "Checking camera.";
	int i_retCode;
	AT_64 iNumberDevices = 0;
	i_retCode = AT_GetInt(AT_HANDLE_SYSTEM, L"DeviceCount", &iNumberDevices);
	if (iNumberDevices <= 0) {
		// No cameras found, check all redistributable binaries
		// have been copied to the executable directory or are in the system path
		// and check atdebug.log file
	} else {
		if (!connected) {
			i_retCode = AT_Open(0, &Hndl);
			if (i_retCode != AT_SUCCESS) {
				//error condition - check atdebug.log
			} else {
				connected = TRUE;
			}
		}
		AT_WC szValue[64];
		i_retCode = AT_GetString(Hndl, L"SerialNumber", szValue, 64);
		if (i_retCode == AT_SUCCESS) {
			//The serial number of the camera is szValue
			qInfo(logInfo()) << "The serial number is" << szValue;
			//wcout << L"The serial number is " << szValue << endl;
		} else {
			//Serial Number feature was not found, check the error code for information
		}
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