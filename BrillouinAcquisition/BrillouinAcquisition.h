#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_BrillouinAcquisition.h"

class BrillouinAcquisition : public QMainWindow {
	Q_OBJECT

private slots:
	void on_actionAbout_triggered();

public:
	BrillouinAcquisition(QWidget *parent = Q_NULLPTR);
	~BrillouinAcquisition();

private:
	Ui::BrillouinAcquisitionClass *ui;
};
