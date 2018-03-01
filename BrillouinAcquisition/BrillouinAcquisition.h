#ifndef BRILLOUINACQUISITON_H
#define BRILLOUINACQUISITON_H

#include "thread.h"
#include "andor.h"
#include "qcustomplot.h"
#include "external/h5bm/h5bm.h"
#include "scancontrol.h"

#include <QtWidgets/QMainWindow>
#include "ui_BrillouinAcquisition.h"

#include <vector>
#include <string>

typedef struct {
	AT_64 left   = 1;
	AT_64 width  = 2048;
	AT_64 bottom = 1;
	AT_64 height = 2048;
	AT_WC *binning = L"1x1";
} CAMERA_ROI;

typedef struct {
	AT_WC *rate			= L"100 MHz";
	AT_WC *sensitivity	= L"16-bit (low noise & high well capacity)";
	AT_WC *encoding		= L"Mono16";
	AT_WC *mode			= L"Fixed";
} CAMERA_READOUT;

typedef struct {
	double exp = 0.5;					// [s]	exposure time
	int nrImages = 2;					// [1]	number of images to acquire at one point
	AT_WC *triggerMode = L"Software";	//		trigger mode
	CAMERA_ROI roi;						//		region of interest
	CAMERA_READOUT readout;				//		readout settings
} SETTINGS_CAMERA;

typedef struct {

} SETTINGS_STAGE;

typedef struct {
	SETTINGS_CAMERA camera;
	SETTINGS_STAGE stage;
} SETTINGS_DEVICES;

enum CustomGradientPreset {
	gpParula
};

Q_DECLARE_METATYPE(std::string);

class BrillouinAcquisition : public QMainWindow {
	Q_OBJECT

private slots:
	void on_actionAbout_triggered();
	void on_camera_singleShot_clicked();
	void on_actionConnect_Camera_triggered();
	void on_actionEnable_Cooling_triggered();
	void on_actionConnect_Stage_triggered();
	void microscopeConnectionChanged(bool);
	void microscopeElementPositionsChanged(std::vector<int>);
	void microscopeElementPositionsChanged(int element, int position);
	void on_camera_playPause_clicked();
	void onNewImage(unsigned short *, AT_64, AT_64);
	void createCameraImage();
	void xAxisRangeChanged(const QCPRange & newRange);
	void yAxisRangeChanged(const QCPRange & newRange);
	void settingsCameraUpdate(SETTINGS_DEVICES);
	void on_ROILeft_valueChanged(int);
	void on_ROIWidth_valueChanged(int);
	void on_ROIBottom_valueChanged(int);
	void on_ROIHeight_valueChanged(int);
	void setColormap(QCPColorGradient *, CustomGradientPreset);
	void setElement(int element, int position);
	void setPreset(int preset);

signals:
	void settingsCameraChanged(SETTINGS_DEVICES);

public:
	BrillouinAcquisition(QWidget *parent = Q_NULLPTR);
	~BrillouinAcquisition();

private:
	Ui::BrillouinAcquisitionClass *ui;
	Thread cameraThread;
	Thread microscopeThread;
	Thread storageThread;
	Andor *andor = new Andor();
	ScanControl *scanControl = new ScanControl();
	QCPColorMap *colorMap;
	SETTINGS_DEVICES settings;
	H5BM *h5bm;
	void writeExampleH5bmFile();
	void checkElementButtons();

	// pre-defined presets for element positions
	// "Brillouin", "Brightfield", "Eyepiece", "Calibration"
	std::vector<std::string> presetLabels = { "Brillouin", "Brightfield", "Eyepiece", "Calibration" };
	std::vector<std::vector<int>> microscope_presets = {
		{ 1, 1, 3, 1, 2, 1 },
		{ 1, 1, 3, 1, 2, 2 },
		{ 1, 1, 3, 2, 3, 2 },
		{ 1, 1, 3, 1, 3, 2 }
	};
	std::vector<int> microscopeElementPositions = {0, 0, 0, 0, 0, 0};

	std::vector<std::vector<QPushButton*>> elementButtons;
	std::vector<QPushButton*> presetButtons;
};

#endif // BRILLOUINACQUISITON_H