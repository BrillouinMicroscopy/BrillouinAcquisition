#ifndef CAMERA_H
#define CAMERA_H

#include <QtCore>
#include <gsl/gsl>

#include "Device.h"

#include "cameraParameters.h"
#include "..\previewBuffer.h"

class Camera : public Device {
	Q_OBJECT

public:
	Camera();
	~Camera();

	bool m_isPreviewRunning{ false };

protected:
	CAMERA_OPTIONS m_options;
	CAMERA_SETTINGS m_settings;

signals:
	void settingsChanged(CAMERA_SETTINGS);
	void optionsChanged(CAMERA_OPTIONS);
	void s_previewBufferSettingsChanged();
};

#endif //CAMERA_H