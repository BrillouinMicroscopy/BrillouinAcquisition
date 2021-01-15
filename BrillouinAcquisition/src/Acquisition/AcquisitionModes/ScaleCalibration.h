#ifndef SCALECALIBRATION_H
#define SCALECALIBRATION_H

#include <QtCore>
#include <gsl/gsl>
#include "H5Cpp.h"

#include "opencv2/imgproc.hpp"

#include "AcquisitionMode.h"
#include "ScaleCalibrationHelper.h"
#include "../../Devices/Cameras/Camera.h"
#include "../../Devices/ScanControls/ScanControl.h"

#include "ui_ScaleCalibrationDialog.h"

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

	void save(std::vector<std::vector<unsigned char>> images, CAMERA_SETTINGS settings, std::vector<POINT2> positions);

	void writePoint(H5::Group group, std::string name, POINT2 point);
	POINT2 readPoint(H5::Group group, const std::string& name);

	void writeAttribute(H5::H5Object& parent, std::string name, double value);
	void writeAttribute(H5::H5Object& parent, std::string name, std::string value);
	void readAttribute(H5::H5Object& parent, std::string name, double* value);

	Camera** m_camera{ nullptr };
	POINT3 m_startPosition{ 0, 0, 0 };

	ScaleCalibrationData m_scaleCalibration;
	double m_Dx{ 10.0 };	// [�m]	shift in x-direction
	double m_Dy{ 10.0 };	// [�m]	shift in y-direction

	Ui::Dialog m_ui;
	QDialog* m_Dialog{ nullptr };

private slots:
	void acquire(std::unique_ptr <StorageWrapper>& storage) override;
	void acquire();

	void on_buttonCancel_clicked();
	void on_buttonApply_clicked();
	void on_buttonAcquire_clicked();
	void closeDialog();

	void setTranslationDistanceX(double dx);
	void setTranslationDistanceY(double dy);

	void updateScaleCalibrationBoxes();

	void setMicrometerToPixX_x(double value);
	void setMicrometerToPixX_y(double value);
	void setMicrometerToPixY_x(double value);
	void setMicrometerToPixY_y(double value);

	void setPixToMicrometerX_x(double value);
	void setPixToMicrometerX_y(double value);
	void setPixToMicrometerY_x(double value);
	void setPixToMicrometerY_y(double value);
};

#endif //SCALECALIBRATION_H
