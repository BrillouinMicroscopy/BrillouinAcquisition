#include "stdafx.h"
#include "Camera.h"

/*
 * Public definitions
 */

CAMERA_OPTIONS Camera::getOptions() {
	return m_options;
}

CAMERA_SETTINGS Camera::getSettings() {
	return m_settings;
}

/*
 * Public slots
 */

void Camera::setSettings(CAMERA_SETTINGS settings) {
	// Don't do anything if an acquisition is running.
	if (m_isAcquisitionRunning) {
		return;
	}

	// Check that pixel encoding is valid
	if (std::find(m_options.pixelEncodings.begin(), m_options.pixelEncodings.end(), settings.readout.pixelEncoding) ==
		m_options.pixelEncodings.end() && !m_options.pixelEncodings.empty()
	) {
		settings.readout.pixelEncoding = m_options.pixelEncodings[0];
	}

	applySettings(settings);
}

void Camera::setSetting(CAMERA_SETTING setting, double value) {
	switch (setting) {
		case CAMERA_SETTING::EXPOSURE:
			m_settings.exposureTime = value;
			break;
		case CAMERA_SETTING::GAIN:
			m_settings.gain = value;
			break;
		default:
			return;
	}
	setSettings(m_settings);
}

void Camera::setSetting(CAMERA_SETTING setting, std::wstring value) {
	switch (setting) {
		case CAMERA_SETTING::ENCODING:
			m_settings.readout.pixelEncoding = value;
			break;
		default:
			return;
	}
	setSettings(m_settings);
}

/*
 * Protected slots
 */

void Camera::getImageForPreview() {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	if (m_isPreviewRunning) {
		if (m_stopPreview) {
			stopPreview();
			return;
		}

		// if no image is ready return immediately
		if (!m_previewBuffer->m_buffer->m_freeBuffers->tryAcquire()) {
			Sleep(50);

			QMetaObject::invokeMethod(this, [this]() { getImageForPreview(); }, Qt::QueuedConnection);
			return;
		}
		acquireImage(m_previewBuffer->m_buffer->getWriteBuffer());
		m_previewBuffer->m_buffer->m_usedBuffers->release();

		QMetaObject::invokeMethod(this, [this]() { getImageForPreview(); }, Qt::QueuedConnection);
	}
}