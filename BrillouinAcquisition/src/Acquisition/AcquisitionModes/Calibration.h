#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <QtCore>
#include <gsl/gsl>

#include "AcquisitionMode.h"
#include "../../Devices/PointGrey.h"
#include "../../Devices/ODTControl.h"

struct CALIBRATION_SETTINGS {
	double Ux_min{ -0.17 };			// [V]	minimum voltage for x-direction
	double Ux_max{ 0.17 };			// [V]	maximum voltage for x-direction
	int Ux_steps{ 20 };				// [1]	number of steps in x-direction
	double Uy_min{ -0.17 };			// [V]	minimum voltage for y-direction
	double Uy_max{ 0.17 };			// [V]	maximum voltage for y-direction
	int Uy_steps{ 20 };				// [1]	number of steps in x-direction
	std::vector<VOLTAGE2> voltages;	// [V]	voltages to apply
};

class Calibration : public AcquisitionMode {
	Q_OBJECT

public:
	Calibration(QObject *parent, Acquisition* acquisition, Camera **camera, ODTControl **ODTControl);
	~Calibration();

public slots:
	void init();
	void initialize();
	void startRepetitions();
	void setCameraSetting(CAMERA_SETTING, double);
	void load(std::string filepath);

	void setWidth(int);
	void setHeight(int);
	void setMagnification(double);
	void setPixelSize(double);

private:
	CALIBRATION_SETTINGS m_acqSettings{};
	CAMERA_SETTINGS m_cameraSettings{ 0.002, 0 };
	Camera **m_camera;
	ODTControl **m_ODTControl;

	void abortMode(std::unique_ptr <StorageWrapper> & storage) override;
	void abortMode();

	double m_minimalIntensity = 100;		// [1] minimum peak intensity for valid peaks

	SpatialCalibration m_calibration;

	double readCalibrationValue(H5::H5File file, std::string datasetName);
	void writeCalibrationValue(H5::Group group, const H5std_string datasetName, double value);
	std::vector<double> readCalibrationMap(H5::H5File file, std::string datasetName);
	void writeCalibrationMap(H5::Group group, std::string datasetName, std::vector<double> map);

	void save();

private slots:
	void acquire(std::unique_ptr <StorageWrapper> & storage) override;
	void acquire() ;

signals:
	void s_cameraSettingsChanged(CAMERA_SETTINGS);				// emit the camera settings
	void calibrationChanged(SpatialCalibration);				// emit the calibration
};

#endif //CALIBRATION_H
