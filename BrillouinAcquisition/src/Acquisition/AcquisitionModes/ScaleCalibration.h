#ifndef SCALECALIBRATION_H
#define SCALECALIBRATION_H

#include <QtCore>
#include <gsl/gsl>
#include "H5Cpp.h"

#include "AcquisitionMode.h"
#include "ScaleCalibrationHelper.h"
#include "../../Devices/Cameras/Camera.h"
#include "../../Devices/ScanControls/ScanControl.h"

class ScaleCalibration : public AcquisitionMode {
	Q_OBJECT

public:
	ScaleCalibration(QObject* parent, Acquisition* acquisition, Camera** camera, ScanControl** scanControl);
	~ScaleCalibration();

public slots:
	void startRepetitions() override;

	void load(std::string filepath);

private:
	void abortMode(std::unique_ptr <StorageWrapper>& storage) override;
	void abortMode();

	void save();

	Camera** m_camera;
	ScanControl** m_scanControl;

	ScaleCalibrationData m_scaleCalibration;

private slots:
	void acquire(std::unique_ptr <StorageWrapper>& storage) override;
	void acquire();
	void writePoint(H5::Group group, std::string name, POINT2 point);
	POINT2 readPoint(H5::Group group, const std::string& name);
};

#endif //SCALECALIBRATION_H
