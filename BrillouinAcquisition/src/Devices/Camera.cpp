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
