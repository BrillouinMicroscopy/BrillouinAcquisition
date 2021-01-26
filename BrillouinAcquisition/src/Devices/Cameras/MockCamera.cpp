#include "stdafx.h"
#include "MockCamera.h"

#include <thread>
#include <limits>

/*
 * Public definitions
 */

MockCamera::~MockCamera() {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
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

void MockCamera::setSettings(CAMERA_SETTINGS settings) {
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

	m_settings.roi.height_binned = m_settings.roi.height_physical / m_settings.roi.binX;
	m_settings.roi.width_binned = m_settings.roi.width_physical / m_settings.roi.binY;

	m_settings.roi.bytesPerFrame = m_settings.roi.height_binned * m_settings.roi.width_binned;
	// Read back the settings
	readSettings();
}

void MockCamera::startPreview() {
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

void MockCamera::stopPreview() {
	m_isPreviewRunning = false;
	m_stopPreview = false;
	emit(s_previewRunning(m_isPreviewRunning));
}

void MockCamera::startAcquisition(CAMERA_SETTINGS settings) {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	// check if currently a preview is running and stop it in case
	if (m_isPreviewRunning) {
		stopPreview();
		m_wasPreviewRunning = true;
	} else {
		m_wasPreviewRunning = false;
	}
	setSettings(settings);

	auto bufferSettings = BUFFER_SETTINGS{ 4, (unsigned int)m_settings.roi.bytesPerFrame, "unsigned char", m_settings.roi };
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

void MockCamera::getImageForAcquisition(unsigned char* buffer, bool preview) {
	std::lock_guard<std::mutex> lockGuard(m_mutex);

	acquireImage(buffer);

	if (preview && buffer != nullptr) {
		// write image to preview buffer
		memcpy(m_previewBuffer->m_buffer->getWriteBuffer(), buffer, m_settings.roi.bytesPerFrame);
		m_previewBuffer->m_buffer->m_usedBuffers->release();
	}
}

void MockCamera::setCalibrationExposureTime(double exposureTime) {
	m_settings.exposureTime = exposureTime;
}

/*
 * Private definitions
 */

int MockCamera::acquireImage(unsigned char* buffer) {

	auto rangeMax = std::numeric_limits<unsigned char>::max();

	// Create test image
	auto data = std::vector<unsigned char>(m_settings.roi.bytesPerFrame, 0.1 * rangeMax);
	auto incX{ 0.35 * rangeMax / m_options.ROIWidthLimits[1] * m_settings.roi.binX };
	auto incY{ 0.35 * rangeMax / m_options.ROIHeightLimits[1] * m_settings.roi.binY };

	for (gsl::index xx{ 0 }; xx < m_settings.roi.width_binned; xx++) {
		for (gsl::index yy{ 0 }; yy < m_settings.roi.height_binned; yy++) {
			auto i = yy * m_settings.roi.width_binned + xx;
			data[i] += (xx + m_settings.roi.left / m_settings.roi.binX) * incX +
				(yy + m_settings.roi.top / m_settings.roi.binY) * incY;
		}
	}


	// Add noise with 10% dynamic range to test image
	auto noiseLvl{ 0.1 };
	auto scaling{ noiseLvl * rangeMax / RAND_MAX };
	for (gsl::index i{ 0 }; i < data.size(); i++) {
		data[i] += scaling * std::rand();
	}

	// Copy data to provided buffer
	if (buffer != nullptr) {
		std::this_thread::sleep_for(std::chrono::milliseconds((int)(1e3*m_settings.exposureTime)));
		memcpy(buffer, &data[0], m_settings.roi.bytesPerFrame);
		return 1;
	}
	return 0;
}

void MockCamera::readOptions() {
	m_options.ROIWidthLimits = {1, 1000};
	m_options.ROIHeightLimits = { 1, 1000 };

	m_options.imageBinnings = {L"1x1", L"2x2", L"4x4", L"8x8"};

	emit(optionsChanged(m_options));
}

void MockCamera::readSettings() {
	// emit signal that settings changed
	emit(settingsChanged(m_settings));
}

void MockCamera::preparePreview() {
	// always use full camera image for live preview
	m_settings.roi.width_physical = m_options.ROIWidthLimits[1];
	m_settings.roi.left = 1;
	m_settings.roi.height_physical = m_options.ROIHeightLimits[1];
	m_settings.roi.top = 1;

	setSettings(m_settings);

	auto bufferSettings = BUFFER_SETTINGS{ 5, (unsigned int)m_settings.roi.bytesPerFrame, "unsigned char", m_settings.roi };
	m_previewBuffer->initializeBuffer(bufferSettings);
	emit(s_previewBufferSettingsChanged());
}