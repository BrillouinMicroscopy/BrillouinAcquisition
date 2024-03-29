#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <QtCore>
#include <gsl/gsl>
#include "H5Cpp.h"

#include "AcquisitionMode.h"
#include "VoltageCalibrationHelper.h"
#include "../../Devices/Cameras/Camera.h"
#include "../../Devices/ScanControls/ODTControl.h"

struct CALIBRATION_SETTINGS {
	double Ux_min{ -0.17 };			// [V]	minimum voltage for x-direction
	double Ux_max{ 0.17 };			// [V]	maximum voltage for x-direction
	int Ux_steps{ 20 };				// [1]	number of steps in x-direction
	double Uy_min{ -0.17 };			// [V]	minimum voltage for y-direction
	double Uy_max{ 0.17 };			// [V]	maximum voltage for y-direction
	int Uy_steps{ 20 };				// [1]	number of steps in x-direction
	std::vector<VOLTAGE2> voltages;	// [V]	voltages to apply
};

class VoltageCalibration : public AcquisitionMode {
	Q_OBJECT

public:
	VoltageCalibration(QObject* parent, Acquisition* acquisition, Camera*& camera, ODTControl*& ODTControl);
	~VoltageCalibration();

public slots:
	void startRepetitions() override;

	void setCameraSetting(CAMERA_SETTING, double);
	void load(std::string filepath);

private:
	void abortMode(std::unique_ptr <StorageWrapper>& storage) override;
	void abortMode();

	double readCalibrationValue(H5::H5File file, std::string datasetName);
	void writeCalibrationValue(H5::Group group, const H5std_string datasetName, double value);
	std::vector<double> readCalibrationMap(H5::H5File file, std::string datasetName);
	void writeCalibrationMap(H5::Group group, std::string datasetName, std::vector<double> map);

	void save();

	template <typename T>
	void __acquire();

	CALIBRATION_SETTINGS m_acqSettings{};
	CAMERA_SETTINGS m_cameraSettings{ 0.002, 0 };
	Camera*& m_camera;
	ODTControl*& m_ODTControl;

	double m_minimalIntensity{ 100 };		// [1] minimum peak intensity for valid peaks

	VoltageCalibrationData m_voltageCalibration;

private slots:
	void acquire();
	void acquire(std::unique_ptr <StorageWrapper>& storage) override;

signals:
	void s_cameraSettingsChanged(CAMERA_SETTINGS);				// emit the camera settings
	void s_voltageCalibrationStatus(std::string title, std::string message);
};

#endif //CALIBRATION_H
