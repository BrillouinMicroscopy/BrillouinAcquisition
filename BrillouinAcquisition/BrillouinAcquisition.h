#pragma once

#include "andor.h"

#include <QtWidgets/QMainWindow>
#include "ui_BrillouinAcquisition.h"

class BrillouinAcquisition : public QMainWindow {
	Q_OBJECT

private slots:
	void on_actionAbout_triggered();
	void on_cameraButton_clicked();

public:
	BrillouinAcquisition(QWidget *parent = Q_NULLPTR);
	~BrillouinAcquisition();

private:
	Ui::BrillouinAcquisitionClass *ui;
	Andor *andor;
};
