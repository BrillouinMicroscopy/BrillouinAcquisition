#ifndef BRILLOUINACQUISITON_H
#define BRILLOUINACQUISITON_H

#include "thread.h"
#include "andor.h"
#include "qcustomplot.h"

#include <QtWidgets/QMainWindow>
#include "ui_BrillouinAcquisition.h"

class BrillouinAcquisition : public QMainWindow {
	Q_OBJECT

private slots:
	void on_actionAbout_triggered();
	void on_camera_singleShot_clicked();
	void on_actionConnect_Camera_triggered();
	void on_actionEnable_Cooling_triggered();
	void on_camera_playPause_clicked();
	void onNewImage(unsigned short *);
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