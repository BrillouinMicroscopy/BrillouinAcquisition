#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <QtCore>
#include <gsl/gsl>

#include "AcquisitionMode.h"
#include "../../Devices/PointGrey.h"
#include "../../Devices/NIDAQ.h"

struct CALIBRATION_SETTINGS {
	double Ux_min{ -0.17 };			// [V]	minimum voltage for x-direction
	double Ux_max{ 0.17 };			// [V]	maximum voltage for x-direction
	int Ux_steps{ 10 };				// [1]	number of steps in x-direction
	double Uy_min{ -0.17 };			// [V]	minimum voltage for y-direction
	double Uy_max{ 0.17 };			// [V]	maximum voltage for y-direction
	int Uy_steps{ 10 };				// [1]	number of steps in x-direction
	std::vector<VOLTAGE2> voltages;	// [V]	voltages to apply
};

class Calibration : public AcquisitionMode {
	Q_OBJECT

public:
	Calibration(QObject *parent, Acquisition* acquisition, Camera **camera, NIDAQ **nidaq);
	~Calibration();

public slots:
	void init();
	void initialize();
	void startRepetitions();
	void setCameraSetting(CAMERA_SETTING, double);
	void loadVoltagePositionCalibration(std::string filepath);

private:
	CALIBRATION_SETTINGS m_acqSettings{};
	CAMERA_SETTINGS m_cameraSettings{ 0.002, 0 };
	Camera **m_camera;
	NIDAQ **m_NIDAQ;

	void abortMode(std::unique_ptr <StorageWrapper> & storage) override;
	void abortMode();

	SpatialCalibration m_calibration;

	double getCalibrationValue(H5::H5File file, std::string datasetName);
	std::vector<double> getCalibrationMap(H5::H5File file, std::string datasetName);

private slots:
	void acquire(std::unique_ptr <StorageWrapper> & storage) override;
	void acquire() ;

signals:
	void s_cameraSettingsChanged(CAMERA_SETTINGS);			// emit the camera settings
};

#endif //CALIBRATION_H