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
	Camera() {};
	~Camera() {};

	bool m_isPreviewRunning{ false };
	bool m_isAcquisitionRunning{ false };

	CAMERA_OPTIONS getOptions();
	CAMERA_SETTINGS getSettings();

public slots:
	virtual void setSettings(CAMERA_SETTINGS) = 0;
	virtual void startPreview() = 0;
	virtual void stopPreview() = 0;
	virtual void startAcquisition(CAMERA_SETTINGS) = 0;
	virtual void stopAcquisition() = 0;

protected:
	CAMERA_OPTIONS m_options;
	CAMERA_SETTINGS m_settings;

	virtual void readOptions() = 0;
	virtual void readSettings() = 0;

signals:
	void settingsChanged(CAMERA_SETTINGS);
	void optionsChanged(CAMERA_OPTIONS);
	void s_previewBufferSettingsChanged();
	void s_previewRunning(bool);
	void s_acquisitionRunning(bool);
};

#endif //CAMERA_H