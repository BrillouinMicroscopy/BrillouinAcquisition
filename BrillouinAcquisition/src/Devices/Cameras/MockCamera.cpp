#include "stdafx.h"
#include "MockCamera.h"

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
	if (!m_isConnected) {
		m_isConnected = true;
	}

	emit(connectedDevice(m_isConnected));
}

void MockCamera::disconnectDevice() {
	if (m_isConnected) {
		m_isConnected = false;
	}

	emit(connectedDevice(m_isConnected));
}

void MockCamera::setSettings(CAMERA_SETTINGS settings) {
	m_settings = settings;
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

/*
 * Private definitions
 */

int MockCamera::acquireImage(unsigned char* buffer) {

	// Copy data to provided buffer
	//if (data != NULL && buffer != nullptr) {
	//	memcpy(buffer, data, m_settings.roi.bytesPerFrame);
	//	return 1;
	//}
	return 0;
}

void MockCamera::readOptions() {
	emit(optionsChanged(m_options));
}

void MockCamera::readSettings() {
	// emit signal that settings changed
	emit(settingsChanged(m_settings));
}