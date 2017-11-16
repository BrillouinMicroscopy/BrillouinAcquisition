#ifndef BRILLOUINACQUISITON_H
#define BRILLOUINACQUISITON_H

#include "thread.h"
#include "andor.h"
#include "qcustomplot.h"

#include <QtWidgets/QMainWindow>
#include "ui_BrillouinAcquisition.h"

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

class BrillouinAcquisition : public QMainWindow {
	Q_OBJECT

private slots:
	void on_actionAbout_triggered();
	void on_camera_singleShot_clicked();
	void on_actionConnect_Camera_triggered();
	void on_actionEnable_Cooling_triggered();
	void on_camera_playPause_clicked();
	void onNewImage(unsigned short *, AT_64, AT_64);
	void createCameraImage();
	void xAxisRangeChanged(const QCPRange & newRange);
	void yAxisRangeChanged(const QCPRange & newRange);

public:
	BrillouinAcquisition(QWidget *parent = Q_NULLPTR);
	~BrillouinAcquisition();

private:
	Ui::BrillouinAcquisitionClass *ui;
	Thread CameraThread;
	Andor *andor = new Andor();
	QCPColorMap *colorMap;
};

#endif // BRILLOUINACQUISITON_H