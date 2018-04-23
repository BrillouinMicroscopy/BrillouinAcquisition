#ifndef BRILLOUINACQUISITON_H
#define BRILLOUINACQUISITON_H

#include "thread.h"
#include "andor.h"
#include "acquisition.h"
#include "qcustomplot.h"
#include "external/h5bm/h5bm.h"
#include "scancontrol.h"

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

Q_DECLARE_METATYPE(std::string);
Q_DECLARE_METATYPE(AT_64);
Q_DECLARE_METATYPE(CircularBuffer<AT_U8>);
Q_DECLARE_METATYPE(ACQUISITION_SETTINGS);
Q_DECLARE_METATYPE(CAMERA_SETTINGS);
Q_DECLARE_METATYPE(std::vector<int>);
Q_DECLARE_METATYPE(QSerialPort::SerialPortError);
Q_DECLARE_METATYPE(IMAGE*);
Q_DECLARE_METATYPE(CALIBRATION*);

class BrillouinAcquisition : public QMainWindow {
	Q_OBJECT

private slots:
	void on_actionAbout_triggered();
	void on_camera_singleShot_clicked();
	void on_actionConnect_Camera_triggered();
	void on_actionEnable_Cooling_triggered();
	void on_actionConnect_Stage_triggered();
	void on_acquisitionStart_clicked();
	void microscopeConnectionChanged(bool);
	void microscopeElementPositionsChanged(std::vector<int>);
	void microscopeElementPositionsChanged(int element, int position);
	void on_camera_playPause_clicked();
	void onNewImage();
	void initializePlot();
	void xAxisRangeChanged(const QCPRange & newRange);
	void yAxisRangeChanged(const QCPRange & newRange);
	void settingsCameraUpdate(SETTINGS_DEVICES);
	void on_ROILeft_valueChanged(int);
	void on_ROIWidth_valueChanged(int);
	void on_ROITop_valueChanged(int);
	void on_ROIHeight_valueChanged(int);
	void setColormap(QCPColorGradient *, CustomGradientPreset);
	void setElement(int element, int position);
	void setPreset(int preset);
	void updatePreview(bool, CircularBuffer<AT_U8>*, AT_64, AT_64);
	void showPreviewRunning(bool);
	void cameraSettingsChanged(CAMERA_SETTINGS);
	void cameraOptionsChanged(CAMERA_OPTIONS);
	void showAcqPosition(double, double, double, int);
	void showAcqProgress(int state, double progress, int seconds);
	void showCalibrationInterval(int);
	void showCalibrationRunning(bool);
	void showAcqRunning(bool);
	void updateFilename(std::string);

	void on_autoscalePlot_stateChanged(int);

	void updateAcquisitionSettings();

	void on_exposureTime_valueChanged(double);
	void on_frameCount_valueChanged(int);

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
	void on_conCalibrationInterval_valueChanged(double);
	void on_nrCalibrationImages_valueChanged(int);
	void on_calibrationExposureTime_valueChanged(double);

signals:
	void settingsCameraChanged(SETTINGS_DEVICES);

public:
	BrillouinAcquisition(QWidget *parent = nullptr);
	~BrillouinAcquisition();

private:
	Ui::BrillouinAcquisitionClass *ui;
	void checkElementButtons();
	void addListToComboBox(QComboBox*, std::vector<AT_WC*>, bool clear = true);
	//Thread m_cameraThread;
	//Thread m_microscopeThread;
	Thread m_acquisitionThread;
	Andor *m_andor = new Andor();
	ScanControl *m_scanControl = new ScanControl();
	Acquisition *m_acquisition = new Acquisition(nullptr, m_andor, m_scanControl);
	QCPColorMap *m_colorMap;
	SETTINGS_DEVICES m_deviceSettings;
	CAMERA_OPTIONS m_cameraOptions;
	ACQUISITION_SETTINGS m_acquisitionSettings;
	bool m_viewRunning = false;
	AT_64 m_imageHeight = 0;
	AT_64 m_imageWidth = 0;
	CircularBuffer<AT_U8> *m_liveBuffer = nullptr;

	bool m_autoscalePlot = false;

	// pre-defined presets for element positions
	// "Brillouin", "Brightfield", "Eyepiece", "Calibration"
	std::vector<std::string> presetLabels = { "Brillouin", "Brightfield", "Eyepiece", "Calibration" };
	std::vector<int> microscopeElementPositions = {0, 0, 0, 0, 0, 0};

	std::vector<std::vector<QPushButton*>> elementButtons;
	std::vector<QPushButton*> presetButtons;
};

#endif // BRILLOUINACQUISITON_H