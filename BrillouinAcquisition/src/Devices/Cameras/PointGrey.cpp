#include "stdafx.h"
#include "PointGrey.h"

/*
 * Public definitions
 */

PointGrey::~PointGrey() {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	disconnectDevice();
}

/*
 * Public slots
 */

void PointGrey::connectDevice() {
	if (!m_isConnected) {
		
		auto numCameras = unsigned int{};
		auto i_retCode = m_busManager.GetNumOfCameras(&numCameras);
		m_numberCameras = numCameras;

		// Check that we stay within the valid range
		if (m_cameraNumber > m_numberCameras - 1) {
			m_cameraNumber = 0;
		}

		if (numCameras > 0) {
			// Select camera

			i_retCode = m_busManager.GetCameraFromIndex((unsigned int)m_cameraNumber, &m_guid);

			i_retCode = m_camera.Connect(&m_guid);

			m_isConnected = true;
			
			readOptions();

			// apply default values for exposure and gain
			m_settings.exposureTime = 0.004;
			m_settings.gain = 0.0;
			setSettings(m_settings);
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
		auto i_retCode = m_camera.Disconnect();

		m_isConnected = false;
	}

	emit(connectedDevice(m_isConnected));
}

void PointGrey::startPreview() {
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

void PointGrey::stopPreview() {
	auto i_retCode = m_camera.StopCapture();
	m_isPreviewRunning = false;
	m_stopPreview = false;
	emit(s_previewRunning(m_isPreviewRunning));
}

void PointGrey::startAcquisition(const CAMERA_SETTINGS& settings) {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	// check if currently a preview is running and stop it in case
	if (m_isPreviewRunning) {
		stopPreview();
		m_wasPreviewRunning = true;
	} else {
		m_wasPreviewRunning = false;
	}
	setSettings(settings);

	auto bufferSettings = BUFFER_SETTINGS{ 1, (unsigned int)m_settings.roi.bytesPerFrame, "unsigned char", m_settings.roi };
	m_previewBuffer->initializeBuffer(bufferSettings);
	emit(s_previewBufferSettingsChanged());

	auto i_retCode = m_camera.StartCapture();
	m_isAcquisitionRunning = true;
	emit(s_acquisitionRunning(m_isAcquisitionRunning));
}

void PointGrey::stopAcquisition() {
	auto i_retCode = m_camera.StopCapture();
	m_isAcquisitionRunning = false;
	emit(s_acquisitionRunning(m_isAcquisitionRunning));

	// Restart the preview if it was running before the acquisition
	if (m_wasPreviewRunning) {
		startPreview();
	}
}

void PointGrey::getImageForAcquisition(std::byte* buffer, bool preview) {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	if (m_settings.readout.triggerMode == L"Software") {
		FireSoftwareTrigger();
	}
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

int PointGrey::acquireImage(std::byte* buffer) {
	auto rawImage = FlyCapture2::Image{};
	auto tmp = m_camera.RetrieveBuffer(&rawImage);

	// Convert the raw image
	auto convertedImage = FlyCapture2::Image{};
	auto i_retCode = rawImage.Convert(FlyCapture2::PIXEL_FORMAT_RAW8, &convertedImage);

	// Get access to raw data
	auto data = static_cast<unsigned char*>(convertedImage.GetData());

	// Copy data to provided buffer
	if (data != NULL && buffer != nullptr) {
		memcpy(buffer, data, m_settings.roi.bytesPerFrame);
		return 1;
	}
	return 0;
}

void PointGrey::readOptions() {

	auto fmt7Info = FlyCapture2::Format7Info{};
	auto supported{ false };
	fmt7Info.mode = FlyCapture2::MODE_0;
	auto i_retCode = m_camera.GetFormat7Info(&fmt7Info, &supported);

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

void PointGrey::readSettings() {

	// Get the camera configuration
	auto config = FlyCapture2::FC2Config{};
	auto i_retCode = m_camera.GetConfiguration(&config);

	/*
	* Get the exposure time
	*/
	auto prop = FlyCapture2::Property{};
	//Define the property to adjust.
	prop.type = FlyCapture2::SHUTTER;
	//Set the property.
	i_retCode = m_camera.GetProperty(&prop);
	// general settings
	m_settings.exposureTime = 1e-3*prop.absValue;	// [s] exposure time

	/*
	 * Get the camera gain
	 */
	auto propGain = FlyCapture2::Property{};
	//Define the property to adjust.
	propGain.type = FlyCapture2::GAIN;
	//Get the property.
	i_retCode = m_camera.GetProperty(&propGain);
	// store property in settings
	m_settings.gain = propGain.absValue;	// [dB] camera gain

	// Create a Format7 Configuration
	auto fmt7ImageSettings = FlyCapture2::Format7ImageSettings{};
	auto packetSize = unsigned int{};
	auto speed = float{};
	i_retCode = m_camera.GetFormat7Configuration(&fmt7ImageSettings, &packetSize, &speed);

	// ROI
	m_settings.roi.height_physical = fmt7ImageSettings.height;
	m_settings.roi.width_physical = fmt7ImageSettings.width;
	m_settings.roi.left = fmt7ImageSettings.offsetX;
	m_settings.roi.top = fmt7ImageSettings.offsetY;

	// readout parameters
	switch (fmt7ImageSettings.pixelFormat) {
		case FlyCapture2::PIXEL_FORMAT_RAW8 :
			m_settings.readout.pixelEncoding = L"Raw8";
			break;
		case FlyCapture2::PIXEL_FORMAT_MONO8:
			m_settings.readout.pixelEncoding = L"Mono8";
			break;
		case FlyCapture2::PIXEL_FORMAT_MONO12:
			m_settings.readout.pixelEncoding = L"Mono12";
			break;
		case FlyCapture2::PIXEL_FORMAT_MONO16:
			m_settings.readout.pixelEncoding = L"Mono16";
			break;
	}

	// read trigger mode
	auto triggerMode = FlyCapture2::TriggerMode{};
	i_retCode = m_camera.GetTriggerMode(&triggerMode);

	/*
	* Get the buffering mode.
	*/
	auto BufferFrame = FlyCapture2::FC2Config{};
	i_retCode = m_camera.GetConfiguration(&BufferFrame);

	// emit signal that settings changed
	emit(settingsChanged(m_settings));
}

void PointGrey::applySettings(const CAMERA_SETTINGS& settings) {
	m_settings = settings;

	if (m_settings.readout.pixelEncoding == L"Raw8") {
		m_settings.readout.dataType = "unsigned char";
	} else if (m_settings.readout.pixelEncoding == L"Mono8") {
		m_settings.readout.dataType = "unsigned char";
	} else if (m_settings.readout.pixelEncoding == L"Mono12") {
		m_settings.readout.dataType = "unsigned short";
	} else if (m_settings.readout.pixelEncoding == L"Mono16") {
		m_settings.readout.dataType = "unsigned short";
	} else {
		// Fallback
		m_settings.readout.pixelEncoding = L"Raw8";
		m_settings.readout.dataType = "unsigned char";
	}

	/*
	* Set the exposure time
	*/
	auto prop = FlyCapture2::Property{};
	//Define the property to adjust.
	prop.type = FlyCapture2::SHUTTER;
	//Ensure the property is on.
	prop.onOff = true;
	// Ensure auto - adjust mode is off.
	prop.autoManualMode = false;
	//Ensure the property is set up to use absolute value control.
	prop.absControl = true;
	//Set the absolute value of shutter
	prop.absValue = 1e3 * m_settings.exposureTime;
	//Set the property.
	auto i_retCode = m_camera.SetProperty(&prop);


	/*
	 * Set the camera gain
	 */
	auto propGain = FlyCapture2::Property{};
	// Define the property to adjust.
	propGain.type = FlyCapture2::GAIN;
	// Ensure auto-adjust mode is off.
	propGain.autoManualMode = false;
	// Ensure the property is set up to use absolute value control.
	propGain.absControl = true;
	//Set the absolute value of gain to 10.5 dB.
	propGain.absValue = m_settings.gain;
	//Set the property.
	i_retCode = m_camera.SetProperty(&propGain);


	/*
	* Set region of interest and pixel format
	*/
	// Create a Format7 Configuration
	auto fmt7ImageSettings = FlyCapture2::Format7ImageSettings{};
	// Acquisition mode is always "MODE_0" for this application
	fmt7ImageSettings.mode = FlyCapture2::MODE_0;
	// Set the pixel format, possible values are: PIXEL_FORMAT_RAW8, PIXEL_FORMAT_MONO8, PIXEL_FORMAT_MONO12, PIXEL_FORMAT_MONO16
	if (m_settings.readout.pixelEncoding == L"Raw8") {
		fmt7ImageSettings.pixelFormat = FlyCapture2::PIXEL_FORMAT_RAW8;
	} else if (m_settings.readout.pixelEncoding == L"Mono8") {
		fmt7ImageSettings.pixelFormat = FlyCapture2::PIXEL_FORMAT_MONO8;
	} else if (m_settings.readout.pixelEncoding == L"Mono12") {
		fmt7ImageSettings.pixelFormat = FlyCapture2::PIXEL_FORMAT_MONO12;
	} else if (m_settings.readout.pixelEncoding == L"Mono16") {
		fmt7ImageSettings.pixelFormat = FlyCapture2::PIXEL_FORMAT_MONO16;
	}

	// Offset x
	fmt7ImageSettings.offsetX = m_settings.roi.left;
	// Offset y
	fmt7ImageSettings.offsetY = m_settings.roi.top;
	// Width
	fmt7ImageSettings.width = m_settings.roi.width_physical;
	// Height
	fmt7ImageSettings.height = m_settings.roi.height_physical;

	m_settings.roi.width_binned = m_settings.roi.width_physical;
	m_settings.roi.height_binned = m_settings.roi.height_physical;

	m_settings.roi.bottom = m_options.ROIHeightLimits[1] - m_settings.roi.top - m_settings.roi.height_physical + 2;
	m_settings.roi.right = m_options.ROIWidthLimits[1] - m_settings.roi.left - m_settings.roi.width_physical + 2;

	m_settings.roi.bytesPerFrame = m_settings.roi.width_binned * m_settings.roi.height_binned;

	auto fmt7PacketInfo = FlyCapture2::Format7PacketInfo{};
	auto valid{ false };
	i_retCode = m_camera.ValidateFormat7Settings(&fmt7ImageSettings, &valid, &fmt7PacketInfo);
	if (valid) {
		i_retCode = m_camera.SetFormat7Configuration(&fmt7ImageSettings, fmt7PacketInfo.recommendedBytesPerPacket);
	}

	/*
	* Set trigger mode
	*/
	auto triggerMode = FlyCapture2::TriggerMode{};
	i_retCode = m_camera.GetTriggerMode(&triggerMode);
	triggerMode.mode = 0;
	triggerMode.parameter = 0;
	triggerMode.polarity = 0;
	if (m_settings.readout.triggerMode == L"Internal") {
		triggerMode.onOff = false;
	} else if (m_settings.readout.triggerMode == L"Software") {
		triggerMode.onOff = true;
		triggerMode.source = 7;	// 7 for software trigger
	} else if (m_settings.readout.triggerMode == L"External") {
		triggerMode.onOff = true;
		triggerMode.source = 0;	// 0 for external trigger
	}

	i_retCode = m_camera.SetTriggerMode(&triggerMode);

	// Wait for software trigger ready
	if (m_settings.readout.triggerMode == L"Software") {
		PollForTriggerReady();
	}

	/*
	* Set the buffering mode.
	*/
	auto BufferFrame = FlyCapture2::FC2Config{};
	i_retCode = m_camera.GetConfiguration(&BufferFrame);
	if (m_settings.readout.cycleMode == L"Fixed") {				// For image preview
		BufferFrame.grabMode = FlyCapture2::DROP_FRAMES;
		BufferFrame.highPerformanceRetrieveBuffer = false;
	} else if (m_settings.readout.cycleMode == L"Continuous") {	// For image preview
		BufferFrame.grabMode = FlyCapture2::BUFFER_FRAMES;
		BufferFrame.highPerformanceRetrieveBuffer = true;
	}
	BufferFrame.numBuffers = m_settings.frameCount;
	i_retCode = m_camera.SetConfiguration(&BufferFrame);

	// Read back the settings
	readSettings();
}

void PointGrey::preparePreview() {
	// set ROI and readout parameters to default preview values, exposure time and gain will be kept
	m_settings.roi.left = 0;
	m_settings.roi.top = 0;
	m_settings.roi.width_physical = m_options.ROIWidthLimits[1];
	m_settings.roi.height_physical = m_options.ROIHeightLimits[1];
	m_settings.readout.pixelEncoding = L"Raw8";
	m_settings.readout.triggerMode = L"Internal";
	m_settings.readout.cycleMode = L"Fixed";
	m_settings.frameCount = 10;

	setSettings(m_settings);

	auto bufferSettings = BUFFER_SETTINGS{ 1, (unsigned int)m_settings.roi.bytesPerFrame, "unsigned char", m_settings.roi };
	m_previewBuffer->initializeBuffer(bufferSettings);
	emit(s_previewBufferSettingsChanged());

	auto i_retCode = m_camera.StartCapture();
}

bool PointGrey::PollForTriggerReady() {
	const auto k_softwareTrigger = unsigned int{ 0x62C };
	auto error = FlyCapture2::Error{};
	auto regVal = unsigned int{ 0 };

	do {
		error = m_camera.ReadRegister(k_softwareTrigger, &regVal);
		if (error != FlyCapture2::PGRERROR_OK) {
			return false;
		}

	} while ((regVal >> 31) != 0);

	return true;
}

bool PointGrey::FireSoftwareTrigger() {
	const auto k_softwareTrigger = unsigned int{ 0x62C };
	const auto k_fireVal = unsigned int{ 0x80000000 };
	auto error = FlyCapture2::Error{};

	error = m_camera.WriteRegister(k_softwareTrigger, k_fireVal);
	if (error != FlyCapture2::PGRERROR_OK) {
		return false;
	}

	return true;
}