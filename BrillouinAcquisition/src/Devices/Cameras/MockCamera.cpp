#include "stdafx.h"
#include "MockCamera.h"

#include <thread>

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

	m_settings.roi.height_binned = m_settings.roi.height_physical;
	m_settings.roi.width_binned = m_settings.roi.width_physical;

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

	auto random = std::rand();

	auto data = std::vector<unsigned char>( m_settings.roi.bytesPerFrame, 0 );
	for (gsl::index i{ 0 }; i < data.size(); i++) {
		data[i] = i + std::rand();
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