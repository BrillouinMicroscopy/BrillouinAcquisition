#ifndef BRILLOUINACQUISITON_H
#define BRILLOUINACQUISITON_H

#include <gsl/gsl>

#include "thread.h"
#include "andor.h"
#include "acquisition.h"
#include "qcustomplot.h"
#include "external/h5bm/h5bm.h"
#include "scancontrol.h"
#include "ZeissECU.h"
#include "NIDAQ.h"

#include <QtWidgets/QMainWindow>
#include "ui_BrillouinAcquisition.h"

#include <vector>
#include <string>

typedef struct {

} STAGE_SETTINGS;

typedef struct {
	CAMERA_SETTINGS camera;
	STAGE_SETTINGS stage;
} SETTINGS_DEVICES;

enum CustomGradientPreset {
	gpParula
};

enum ROI_SOURCE {
	BOX,
	PLOT
};

Q_DECLARE_METATYPE(std::string);
Q_DECLARE_METATYPE(AT_64);
Q_DECLARE_METATYPE(ACQUISITION_SETTINGS);
Q_DECLARE_METATYPE(CAMERA_SETTINGS);
Q_DECLARE_METATYPE(CAMERA_OPTIONS);
Q_DECLARE_METATYPE(std::vector<int>);
Q_DECLARE_METATYPE(QSerialPort::SerialPortError);
Q_DECLARE_METATYPE(IMAGE*);
Q_DECLARE_METATYPE(CALIBRATION*);
Q_DECLARE_METATYPE(ScanControl::SCAN_PRESET);
Q_DECLARE_METATYPE(ScanControl::DEVICE_ELEMENT);

class BrillouinAcquisition : public QMainWindow {
	Q_OBJECT

private slots:
	void showEvent(QShowEvent* event);
	void on_actionAbout_triggered();
	void on_camera_singleShot_clicked();
	// connect camera and react
	void on_actionConnect_Camera_triggered();
	void cameraConnectionChanged(bool);
	void showNoCameraFound();
	// enable camera cooling and react
	void on_actionEnable_Cooling_triggered();
	void cameraCoolingChanged(bool);
	// connect microscope and react
	void on_actionConnect_Stage_triggered();
	void microscopeConnectionChanged(bool);

	void on_acquisitionStart_clicked();
	void microscopeElementPositionsChanged(std::vector<int>);
	void microscopeElementPositionChanged(ScanControl::DEVICE_ELEMENT element, int position);
	void on_camera_playPause_clicked();
	void onNewImage();
	void initializePlot();

	// set and check camera ROI
	void xAxisRangeChanged(const QCPRange & newRange);
	void yAxisRangeChanged(const QCPRange & newRange);
	void on_ROILeft_valueChanged(int);
	void on_ROIWidth_valueChanged(int);
	void on_ROITop_valueChanged(int);
	void on_ROIHeight_valueChanged(int);
	void settingsCameraUpdate(int);
	std::vector<AT_64> checkROI(std::vector<AT_64>, std::vector<AT_64>);

	void setColormap(QCPColorGradient *, CustomGradientPreset);
	void setElement(ScanControl::DEVICE_ELEMENT element, int position);
	void setPreset(ScanControl::SCAN_PRESET preset);
	void updatePreview();
	void showPreviewRunning(bool);
	void startPreview(bool);
	void cameraSettingsChanged(CAMERA_SETTINGS);
	void cameraOptionsChanged(CAMERA_OPTIONS);
	void showAcqPosition(double, double, double, int);
	void showAcqProgress(int state, double progress, int seconds);
	void showCalibrationInterval(int);
	void showCalibrationRunning(bool);
	void showAcqRunning(bool);
	void updateFilename(std::string);

	QString formatSeconds(int seconds);

	void on_actionQuit_triggered();

	void on_autoscalePlot_stateChanged(int);

	void updateAcquisitionSettings();

	void on_exposureTime_valueChanged(double);
	void on_frameCount_valueChanged(int);

	void on_selectFolder_clicked();

	// acquisition AOI
	void on_startX_valueChanged(double);
	void on_startY_valueChanged(double);
	void on_startZ_valueChanged(double);
	void on_endX_valueChanged(double);
	void on_endY_valueChanged(double);
	void on_endZ_valueChanged(double);
	void on_stepsX_valueChanged(int);
	void on_stepsY_valueChanged(int);
	void on_stepsZ_valueChanged(int);

	// live calibration
	void on_preCalibration_stateChanged(int);
	void on_postCalibration_stateChanged(int);
	void on_conCalibration_stateChanged(int);
	void on_sampleSelection_currentIndexChanged(const QString &text);
	void on_conCalibrationInterval_valueChanged(double);
	void on_nrCalibrationImages_valueChanged(int);
	void on_calibrationExposureTime_valueChanged(double);

	// repetitions
	void on_repetitionCount_valueChanged(int);
	void on_repetitionInterval_valueChanged(double);
	void showRepProgress(int repNumber, int timeToNext);

public:
	BrillouinAcquisition(QWidget *parent = nullptr) noexcept;
	~BrillouinAcquisition();

private:
	Ui::BrillouinAcquisitionClass *ui;
	void checkElementButtons();
	void addListToComboBox(QComboBox*, std::vector<AT_WC*>, bool clear = true);
	//Thread m_cameraThread;
	//Thread m_microscopeThread;
	Thread m_acquisitionThread;
	Andor *m_andor = new Andor();
	ScanControl *m_scanControl = nullptr;
	Acquisition *m_acquisition = new Acquisition(nullptr, m_andor, &m_scanControl);
	QCPColorMap *m_colorMap;
	QCPRange m_cLim_Default = { 100, 300 };	// default colormap range
	SETTINGS_DEVICES m_deviceSettings;
	CAMERA_OPTIONS m_cameraOptions;
	ACQUISITION_SETTINGS m_acquisitionSettings;
	bool m_previewRunning = false;
	bool m_measurementRunning = false;

	bool m_autoscalePlot = false;

	std::vector<int> m_deviceElementPositions = {0, 0, 0, 0, 0, 0};

	std::vector<std::vector<QPushButton*>> elementButtons;
	std::vector<QPushButton*> presetButtons;
};

#endif // BRILLOUINACQUISITON_H