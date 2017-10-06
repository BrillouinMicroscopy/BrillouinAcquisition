#ifndef BRILLOUINACQUISITON_H
#define BRILLOUINACQUISITON_H

#include "thread.h"
#include "andor.h"

#include <QtWidgets/QMainWindow>
#include "ui_BrillouinAcquisition.h"

class BrillouinAcquisition : public QMainWindow {
	Q_OBJECT

private slots:
	void on_actionAbout_triggered();
	void on_cameraButton_clicked();
	void on_actionConnect_Camera_triggered();
	void on_actionEnable_Cooling_triggered();

public:
	BrillouinAcquisition(QWidget *parent = Q_NULLPTR);
	~BrillouinAcquisition();

private:
	Ui::BrillouinAcquisitionClass *ui;
	Thread CameraThread;
	Andor *andor;
};

#endif // BRILLOUINACQUISITON_H