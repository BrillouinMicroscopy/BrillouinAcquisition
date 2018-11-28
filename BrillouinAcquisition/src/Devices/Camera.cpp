#include "stdafx.h"
#include "Camera.h"

CAMERA_OPTIONS Camera::getOptions() {
	return m_options;
}

CAMERA_SETTINGS Camera::getSettings() {
	return m_settings;
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

void Camera::getImageForPreview() {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	if (m_isPreviewRunning) {
		if (m_stopPreview) {
			stopPreview();
			return;
		}

		m_previewBuffer->m_buffer->m_freeBuffers->acquire();
		acquireImage(m_previewBuffer->m_buffer->getWriteBuffer());
		m_previewBuffer->m_buffer->m_usedBuffers->release();

		QMetaObject::invokeMethod(this, "getImageForPreview", Qt::QueuedConnection);
	}
}