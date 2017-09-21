#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_BrillouinAcquisition.h"

class BrillouinAcquisition : public QMainWindow {
	Q_OBJECT

public:
	BrillouinAcquisition(QWidget *parent = Q_NULLPTR);

private:
	Ui::BrillouinAcquisitionClass ui;
};
