#include "stdafx.h"
#include "MockCamera.h"

#include <thread>
#include <limits>

/*
 * Public definitions
 */

MockCamera::~MockCamera() {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	disconnectDevice();
}

/*
 * Public slots
 */

void MockCamera::connectDevice() {
	m_isConnected = true;
	readOptions();

	emit(connectedDevice(m_isConnected));

	auto sensor = SensorTemperature{ 0, 0, 0, 0, enCameraTemperatureStatus::STABILISED };
	emit(s_sensorTemperatureChanged(sensor));
}

void MockCamera::disconnectDevice() {
	m_isConnected = false;

	emit(connectedDevice(m_isConnected));
}

void MockCamera::startPreview() {
	// don't do anything if an acquisition is running
	if (m_isAcquisitionRunning) {
		return;
	}
	preparePreview();
	m_isPreviewRunning = true;
	m_stopPreview = false;
	getImageForPreview();

	emit(s_previewRunning(m_isPreviewRunning));
}

void MockCamera::stopPreview() {
	m_isPreviewRunning = false;
	m_stopPreview = false;
	emit(s_previewRunning(m_isPreviewRunning));
}

void MockCamera::startAcquisition(const CAMERA_SETTINGS& settings) {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	// check if currently a preview is running and stop it in case
	if (m_isPreviewRunning) {
		stopPreview();
		m_wasPreviewRunning = true;
	} else {
		m_wasPreviewRunning = false;
	}
	setSettings(settings);

	auto bufferSettings = BUFFER_SETTINGS{ 4, (unsigned int)m_settings.roi.bytesPerFrame, m_settings.readout.dataType, m_settings.roi };
	m_previewBuffer->initializeBuffer(bufferSettings);

	emit(s_previewBufferSettingsChanged());

	m_isAcquisitionRunning = true;
	emit(s_acquisitionRunning(m_isAcquisitionRunning));
}

void MockCamera::stopAcquisition() {
	m_isAcquisitionRunning = false;
	emit(s_acquisitionRunning(m_isAcquisitionRunning));

	// Restart the preview if it was running before the acquisition
	if (m_wasPreviewRunning) {
		startPreview();
	}
}

void MockCamera::getImageForAcquisition(std::byte* buffer, bool preview) {
	std::lock_guard<std::mutex> lockGuard(m_mutex);

	acquireImage(buffer);

	if (preview && buffer != nullptr) {
		// write image to preview buffer
		memcpy(m_previewBuffer->m_buffer->getWriteBuffer(), buffer, m_settings.roi.bytesPerFrame);
		m_previewBuffer->m_buffer->m_usedBuffers->release();
		emit(s_imageReady());
	}
}

void MockCamera::setCalibrationExposureTime(double exposureTime) {
	m_settings.exposureTime = exposureTime;
}

/*
 * Private definitions
 */

template <typename T>
int MockCamera::acquireImage(std::byte* buffer) {

	if (buffer == nullptr) {
		return 0;
	}

	auto rangeMax{ 0 };
	if (m_settings.readout.pixelEncoding == L"16 bit") {
		rangeMax = 65536;
	} else if (m_settings.readout.pixelEncoding == L"12 bit") {
		rangeMax = 4096;
	} else if (m_settings.readout.pixelEncoding == L"8 bit") {
		rangeMax = 255;
	} else {
		rangeMax = 255;
	}

	// Create test image
	auto incX{ 0.35 * rangeMax / m_options.ROIWidthLimits[1] * m_settings.roi.binX };
	auto incY{ 0.35 * rangeMax / m_options.ROIHeightLimits[1] * m_settings.roi.binY };

	auto noiseLvl{ 0.1 };
	auto scaling{ noiseLvl * rangeMax / RAND_MAX };

	auto typeSize = sizeof(T);
	auto value = T{};
	for (gsl::index xx{ 0 }; xx < m_settings.roi.width_binned; xx++) {
		for (gsl::index yy{ 0 }; yy < m_settings.roi.height_binned; yy++) {

			value = (T)(0.1 * rangeMax + (xx + m_settings.roi.left / m_settings.roi.binX) * incX +
				(yy + m_settings.roi.top / m_settings.roi.binY) * incY + scaling * std::rand());

			memcpy(&buffer[typeSize * (yy * m_settings.roi.width_binned + xx)], &value, typeSize);
		}
	}

	// Sleep for exposure time
	std::this_thread::sleep_for(std::chrono::milliseconds((int)(1e3 * m_settings.exposureTime)));
	return 1;
}

int MockCamera::acquireImage(std::byte* buffer) {
	if (m_settings.readout.dataType == "unsigned short") {
		return acquireImage<unsigned short>(buffer);
	} if (m_settings.readout.dataType == "unsigned char") {
		return acquireImage<unsigned char>(buffer);
	}
	return 0;
}

void MockCamera::readOptions() {
	m_options.ROIWidthLimits = {1, 1000};
	m_options.ROIHeightLimits = { 1, 1000 };

	m_options.pixelEncodings = {L"8 bit", L"12 bit", L"16 bit"};

	m_options.imageBinnings = {L"1x1", L"2x2", L"4x4", L"8x8"};

	emit(optionsChanged(m_options));
}

void MockCamera::readSettings() {
	// emit signal that settings changed
	emit(settingsChanged(m_settings));
}

void MockCamera::applySettings(const CAMERA_SETTINGS& settings) {
	m_settings = settings;

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

	m_settings.roi.bottom = m_options.ROIHeightLimits[1] - m_settings.roi.top - m_settings.roi.height_physical + 2;
	m_settings.roi.right = m_options.ROIWidthLimits[1] - m_settings.roi.left - m_settings.roi.width_physical + 2;

	m_settings.roi.height_binned = m_settings.roi.height_physical / m_settings.roi.binX;
	m_settings.roi.width_binned = m_settings.roi.width_physical / m_settings.roi.binY;

	m_settings.roi.bytesPerFrame = m_settings.roi.height_binned * m_settings.roi.width_binned;

	if (m_settings.readout.pixelEncoding == L"16 bit") {
		m_settings.readout.dataType = "unsigned short";
		m_settings.roi.bytesPerFrame *= sizeof(unsigned short);
	} else if (m_settings.readout.pixelEncoding == L"12 bit") {
		m_settings.readout.dataType = "unsigned short";
		m_settings.roi.bytesPerFrame *= sizeof(unsigned short);
	} else if (m_settings.readout.pixelEncoding == L"8 bit") {
		m_settings.readout.dataType = "unsigned char";
		m_settings.roi.bytesPerFrame *= sizeof(unsigned char);
	} else {
		// Fallback
		m_settings.readout.pixelEncoding = L"8 bit";
		m_settings.readout.dataType = "unsigned char";
		m_settings.roi.bytesPerFrame *= sizeof(unsigned char);
	}

	// Read back the settings
	readSettings();

	if (m_isPreviewRunning) {
		preparePreviewBuffer();
	}
}

void MockCamera::preparePreview() {
	// always use full camera image for live preview
	m_settings.roi.width_physical = m_options.ROIWidthLimits[1];
	m_settings.roi.left = 1;
	m_settings.roi.height_physical = m_options.ROIHeightLimits[1];
	m_settings.roi.top = 1;

	setSettings(m_settings);

	preparePreviewBuffer();
}

void MockCamera::preparePreviewBuffer() {
	auto bufferSettings = BUFFER_SETTINGS{ 5, (unsigned int)m_settings.roi.bytesPerFrame, m_settings.readout.dataType, m_settings.roi };
	m_previewBuffer->initializeBuffer(bufferSettings);
	emit(s_previewBufferSettingsChanged());
}