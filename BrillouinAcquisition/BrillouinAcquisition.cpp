#include "stdafx.h"
#include "BrillouinAcquisition.h"
#include "version.h"
#include "logger.h"


BrillouinAcquisition::BrillouinAcquisition(QWidget *parent):
	QMainWindow(parent), ui(new Ui::BrillouinAcquisitionClass) {
	ui->setupUi(this);

	QIcon icon(":/BrillouinAcquisition/assets/00disconnected.png");
	ui->settingsWidget->setTabIcon(0, icon);
	ui->settingsWidget->setTabIcon(1, icon);
	ui->settingsWidget->setIconSize(QSize(16, 16));

	andor = new Andor();

	CameraThread.startWorker(andor);
}

BrillouinAcquisition::~BrillouinAcquisition() {
	CameraThread.exit();
	delete andor;
	qInfo(logInfo()) << "BrillouinAcquisition closed.";
	delete ui;
}

void BrillouinAcquisition::on_actionConnect_Camera_triggered() {
	if (andor->getConnectionStatus()) {
		andor->disconnect();
		ui->actionConnect_Camera->setText("Connect Camera");
		QIcon icon(":/BrillouinAcquisition/assets/00disconnected.png");
		ui->settingsWidget->setTabIcon(0, icon);
	} else {
		andor->connect();
		ui->actionConnect_Camera->setText("Disconnect Camera");
		QIcon icon(":/BrillouinAcquisition/assets/01standby.png");
		ui->settingsWidget->setTabIcon(0, icon);
	}
}

void BrillouinAcquisition::on_actionAbout_triggered() {
	QString clean = "Yes";
	if (Version::VerDirty) {
		clean = "No";
	}
	std::string preRelease = "";
	if (Version::PRERELEASE.length() > 0) {
		preRelease = "-" + Version::PRERELEASE;
	}
	QString str = QString("BrillouinAcquisition Version %1.%2.%3%4 <br> Build from commit: <a href='%5'>%6</a><br>Clean build: %7<br>Author: <a href='mailto:%8?subject=BrillouinAcquisition'>%9</a><br>Date: %10")
		.arg(Version::MAJOR)
		.arg(Version::MINOR)
		.arg(Version::PATCH)
		.arg(preRelease.c_str())
		.arg(Version::Url.c_str())
		.arg(Version::Commit.c_str())
		.arg(clean)
		.arg(Version::AuthorEmail.c_str())
		.arg(Version::Author.c_str())
		.arg(Version::Date.c_str());

	QMessageBox::about(this, tr("About BrillouinAcquisition"), str);
}

void BrillouinAcquisition::on_cameraButton_clicked() {
	qDebug(logDebug()) << "Camera Button clicked";
	QMetaObject::invokeMethod(andor, "getImages", Qt::QueuedConnection);
	qDebug(logDebug()) << "Measurement done.";
}