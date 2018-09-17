#include "stdafx.h"
#include "PointGrey.h"

PointGrey::~PointGrey() {
	disconnectDevice();
}

void PointGrey::connectDevice() {
	if (!m_isConnected) {
		
		unsigned int numCameras;
		m_busManager.GetNumOfCameras(&numCameras);

		if (numCameras > 0) {
			// Select camera

			m_busManager.GetCameraFromIndex(0, &m_guid);

			m_camera.Connect(&m_guid);

			m_isConnected = true;
			
			readOptions();
			readSettings();

		}
	}

	emit(connectedDevice(m_isConnected));
}

void PointGrey::disconnectDevice() {
	if (m_isConnected) {
		if (m_isPreviewRunning) {
			stopPreview();
		}

		// Deinitialize camera
		m_camera.Disconnect();

		m_isConnected = false;
	}

	emit(connectedDevice(m_isConnected));
}

void PointGrey::readOptions() {

	Format7Info fmt7Info;
	bool supported;
	fmt7Info.mode = FlyCapture2::MODE_0;
	m_camera.GetFormat7Info(&fmt7Info, &supported);

	m_options.pixelEncodings = { L"Raw8", L"Mono8", L"Mono12", L"Mono16" };
	//m_options.cycleModes = { L"Continuous", L"Fixed single", L"Fixed multiple" };

	m_options.exposureTimeLimits[0] = 1e-3;
	m_options.exposureTimeLimits[1] = 1;

	m_options.ROIWidthLimits[0] = 1;
	m_options.ROIWidthLimits[1] = fmt7Info.maxWidth;

	m_options.ROIHeightLimits[0] = 1;
	m_options.ROIHeightLimits[1] = fmt7Info.maxHeight;

	emit(optionsChanged(m_options));
}

CAMERA_SETTINGS PointGrey::readSettings() {

	// Get the camera configuration
	FC2Config config;
	m_camera.GetConfiguration(&config);

	/*
	* Get the exposure time
	*/
	Property prop;
	//Define the property to adjust.
	prop.type = SHUTTER;
	//Set the property.
	m_camera.GetProperty(&prop);
	// general settings
	m_settings.exposureTime = 1e-3*prop.absValue;	// [s] exposure time


	// Create a Format7 Configuration
	Format7ImageSettings fmt7ImageSettings;
	unsigned int packetSize;
	float speed;
	m_camera.GetFormat7Configuration(&fmt7ImageSettings, &packetSize, &speed);

	// ROI
	m_settings.roi.height = fmt7ImageSettings.height;
	m_settings.roi.width = fmt7ImageSettings.width;
	m_settings.roi.left = fmt7ImageSettings.offsetX;
	m_settings.roi.top = fmt7ImageSettings.offsetY;

	// readout parameters
	switch (fmt7ImageSettings.pixelFormat) {
		case PIXEL_FORMAT_RAW8 :
			m_settings.readout.pixelEncoding = L"Raw8";
			break;
		case PIXEL_FORMAT_MONO8:
			m_settings.readout.pixelEncoding = L"Mono8";
			break;
		case PIXEL_FORMAT_MONO12:
			m_settings.readout.pixelEncoding = L"Mono12";
			break;
		case PIXEL_FORMAT_MONO16:
			m_settings.readout.pixelEncoding = L"Mono16";
			break;
	}

	//AcquisitionModeEnums cycleMode = m_camera->AcquisitionMode.GetValue();
	//switch (cycleMode) {
	//	case AcquisitionMode_Continuous:
	//		m_settings.readout.cycleMode = L"Continuous";
	//		break;
	//	case AcquisitionMode_SingleFrame:
	//		m_settings.readout.cycleMode = L"Fixed single";
	//		break;
	//	case AcquisitionMode_MultiFrame:
	//		m_settings.readout.cycleMode = L"Fixed multiple";
	//		break;
	//}

	// emit signal that settings changed
	emit(settingsChanged(m_settings));

	return m_settings;
};

bool PointGrey::getConnectionStatus() {
	return m_isConnected;
}

void PointGrey::setSettingsPreview() {
	/*
	 * Set acquisition to continuous
	 */
	//m_camera->AcquisitionMode.SetValue(AcquisitionModeEnums::AcquisitionMode_Continuous);

	/*
	 * Set the exposure time
	 */
	Property prop;
	//Define the property to adjust.
	prop.type = SHUTTER;
	//Ensure the property is on.
	prop.onOff = true;
	// Ensure auto - adjust mode is off.
	prop.autoManualMode = false;
	//Ensure the property is set up to use absolute value control.
	prop.absControl = true;
	//Set the absolute value of shutter to 1 ms.
	m_settings.exposureTime = 0.005;	// [s]
	prop.absValue = 1e3*m_settings.exposureTime;
	//Set the property.
	m_camera.SetProperty(&prop);

	/*
	 * Set ROI and pixel format
	 */
	// Create a Format7 Configuration
	Format7ImageSettings fmt7ImageSettings;

	Mode fmt7Mode = MODE_0;
	fmt7ImageSettings.mode = fmt7Mode;
	// Possible values: PIXEL_FORMAT_RAW8, PIXEL_FORMAT_MONO8, PIXEL_FORMAT_MONO12, PIXEL_FORMAT_MONO16
	fmt7ImageSettings.pixelFormat = PIXEL_FORMAT_RAW8;

	// Offset x to minimum
	m_settings.roi.left = 0;
	fmt7ImageSettings.offsetX = m_settings.roi.left;
	// Offset y to minimum
	m_settings.roi.top = 0;
	fmt7ImageSettings.offsetY = m_settings.roi.top;
	// Width to maximum
	m_settings.roi.width = m_options.ROIWidthLimits[1];
	fmt7ImageSettings.width = m_settings.roi.width;
	// Height to maximum
	m_settings.roi.height = m_options.ROIHeightLimits[1];
	fmt7ImageSettings.height = m_settings.roi.height;

	Format7PacketInfo fmt7PacketInfo;
	bool valid;
	m_camera.ValidateFormat7Settings(&fmt7ImageSettings, &valid, &fmt7PacketInfo);
	if (valid) {
		m_camera.SetFormat7Configuration(&fmt7ImageSettings, fmt7PacketInfo.recommendedBytesPerPacket);
	}

	/*
	 * Set trigger mode
	 */
	TriggerMode triggerMode;
	m_camera.GetTriggerMode(&triggerMode);
	triggerMode.onOff = true;
	triggerMode.source = 7;	// 7 for software trigger
	triggerMode.mode = 0;
	triggerMode.parameter = 0;

	m_camera.SetTriggerMode(&triggerMode);

	PollForTriggerReady(&m_camera);

	/*
	* Set the buffering mode.
	*/
	FC2Config BufferFrame;
	m_camera.GetConfiguration(&BufferFrame);
	BufferFrame.grabMode = DROP_FRAMES;
	BufferFrame.numBuffers = 10;
	BufferFrame.highPerformanceRetrieveBuffer = false;
	m_camera.SetConfiguration(&BufferFrame);

	// Read the settings back
	readSettings();
}

void PointGrey::setSettingsMeasurement() {
	/*
	* Set acquisition to continuous
	*/
	//m_camera->AcquisitionMode.SetValue(AcquisitionModeEnums::AcquisitionMode_Continuous);

	/*
	* Set the exposure time
	*/
	Property prop;
	//Define the property to adjust.
	prop.type = SHUTTER;
	//Ensure the property is on.
	prop.onOff = true;
	// Ensure auto - adjust mode is off.
	prop.autoManualMode = false;
	//Ensure the property is set up to use absolute value control.
	prop.absControl = true;
	//Set the absolute value of shutter to 1 ms.
	m_settings.exposureTime = 0.005;	// [s]
	prop.absValue = 1e3*m_settings.exposureTime;
	//Set the property.
	m_camera.SetProperty(&prop);

	/*
	* Set ROI and pixel format
	*/
	// Create a Format7 Configuration
	Format7ImageSettings fmt7ImageSettings;

	Mode fmt7Mode = MODE_0;
	fmt7ImageSettings.mode = fmt7Mode;
	// Possible values: PIXEL_FORMAT_RAW8, PIXEL_FORMAT_MONO8, PIXEL_FORMAT_MONO12, PIXEL_FORMAT_MONO16
	fmt7ImageSettings.pixelFormat = PIXEL_FORMAT_RAW8;

	// Offset x to minimum
	m_settings.roi.left = 128;
	fmt7ImageSettings.offsetX = m_settings.roi.left;
	// Offset y to minimum
	m_settings.roi.top = 0;
	fmt7ImageSettings.offsetY = m_settings.roi.top;
	// Width to maximum
	m_settings.roi.width = 1024;
	fmt7ImageSettings.width = m_settings.roi.width;
	// Height to maximum
	m_settings.roi.height = m_options.ROIHeightLimits[1];
	fmt7ImageSettings.height = m_settings.roi.height;

	Format7PacketInfo fmt7PacketInfo;
	bool valid;
	m_camera.ValidateFormat7Settings(&fmt7ImageSettings, &valid, &fmt7PacketInfo);
	if (valid) {
		m_camera.SetFormat7Configuration(&fmt7ImageSettings, fmt7PacketInfo.recommendedBytesPerPacket);
	}

	/*
	* Set trigger mode
	*/
	TriggerMode triggerMode;
	m_camera.GetTriggerMode(&triggerMode);
	triggerMode.onOff = true;
	triggerMode.source = 0;	// 7 for software trigger
	triggerMode.mode = 0;
	triggerMode.parameter = 0;
	triggerMode.polarity = 0;

	m_camera.SetTriggerMode(&triggerMode);

	/*
	 * Set the buffering mode.
	 */
	FC2Config BufferFrame;
	m_camera.GetConfiguration(&BufferFrame);
	BufferFrame.grabMode = BUFFER_FRAMES;
	BufferFrame.numBuffers = 150;
	BufferFrame.highPerformanceRetrieveBuffer = true;
	m_camera.SetConfiguration(&BufferFrame);

	// Read the settings back
	readSettings();
}

void PointGrey::startPreview(CAMERA_SETTINGS settings) {
	m_isPreviewRunning = true;
	m_settings = settings;
	preparePreview();
	getImageForPreview();
}

void PointGrey::preparePreview() {
	// always use full camera image for live preview
	m_settings.roi.width = m_options.ROIWidthLimits[1];
	m_settings.roi.left = 1;
	m_settings.roi.height = m_options.ROIHeightLimits[1];
	m_settings.roi.top = 1;

	setSettingsPreview();

	int pixelNumber = m_settings.roi.width * m_settings.roi.height;
	BUFFER_SETTINGS bufferSettings = { 4, pixelNumber, m_settings.roi };
	previewBuffer->initializeBuffer(bufferSettings);
	emit(s_previewBufferSettingsChanged());

	m_camera.StartCapture();

	emit(s_previewRunning(true));
}

void PointGrey::stopPreview() {
	m_isPreviewRunning = false;
	cleanupAcquisition();
	emit(s_previewRunning(false));
}

void PointGrey::cleanupAcquisition() {
	m_camera.StopCapture();
}

void PointGrey::readImageFromCamera(unsigned char * buffer) {
	Image rawImage;
	Error tmp = m_camera.RetrieveBuffer(&rawImage);

	// Convert the raw image
	Image convertedImage;
	rawImage.Convert(PIXEL_FORMAT_RAW8, &convertedImage);

	// Get access to raw data
	unsigned char* data = static_cast<unsigned char*>(convertedImage.GetData());

	// Copy data to preview buffer
	if (data != NULL) {
		memcpy(buffer, data, m_settings.roi.width*m_settings.roi.height);
	}
}

void PointGrey::getImageForPreview() {
	if (m_isPreviewRunning) {

		previewBuffer->m_buffer->m_freeBuffers->acquire();
		acquireImage(previewBuffer->m_buffer->getWriteBuffer());
		previewBuffer->m_buffer->m_usedBuffers->release();

		QMetaObject::invokeMethod(this, "getImageForPreview", Qt::QueuedConnection);
	} else {
		stopPreview();
	}
}

void PointGrey::acquireImage(unsigned char* buffer) {

	PollForTriggerReady(&m_camera);

	// Fire the software trigger
	FireSoftwareTrigger(&m_camera);

	readImageFromCamera(buffer);
};

bool PointGrey::PollForTriggerReady(Camera *camera) {
	const unsigned int k_softwareTrigger = 0x62C;
	Error error;
	unsigned int regVal = 0;

	do {
		error = camera->ReadRegister(k_softwareTrigger, &regVal);
		if (error != PGRERROR_OK) {
			return false;
		}

	} while ((regVal >> 31) != 0);

	return true;
}

bool PointGrey::FireSoftwareTrigger(Camera *camera) {
	const unsigned int k_softwareTrigger = 0x62C;
	const unsigned int k_fireVal = 0x80000000;
	Error error;

	error = camera->WriteRegister(k_softwareTrigger, k_fireVal);
	if (error != PGRERROR_OK) {
		return false;
	}

	return true;
}