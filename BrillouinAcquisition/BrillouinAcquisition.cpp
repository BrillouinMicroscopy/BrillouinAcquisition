#include "stdafx.h"
#include "BrillouinAcquisition.h"
#include "version.h"
#include "logger.h"


BrillouinAcquisition::BrillouinAcquisition(QWidget *parent):
	QMainWindow(parent), ui(new Ui::BrillouinAcquisitionClass) {
	ui->setupUi(this);

	// slot for newly acquired images
	QWidget::connect(
		andor,
		SIGNAL(imageAcquired(unsigned short*, AT_64, AT_64)),
		this,
		SLOT(onNewImage(unsigned short*, AT_64, AT_64))
	);

	// slot to limit the axis of the camera display after user interaction
	QWidget::connect(
		ui->customplot->xAxis,
		SIGNAL(rangeChanged(QCPRange)),
		this,
		SLOT(xAxisRangeChanged(QCPRange))
	);
	QWidget::connect(
		ui->customplot->yAxis,
		SIGNAL(rangeChanged(QCPRange)),
		this,
		SLOT(yAxisRangeChanged(QCPRange))
	);

	QIcon icon(":/BrillouinAcquisition/assets/00disconnected.png");
	ui->settingsWidget->setTabIcon(0, icon);
	ui->settingsWidget->setTabIcon(1, icon);
	ui->settingsWidget->setIconSize(QSize(16, 16));

	ui->actionEnable_Cooling->setEnabled(FALSE);

	// start camera worker thread
	CameraThread.startWorker(andor);

	// set up the camera image plot
	BrillouinAcquisition::createCameraImage();
}

BrillouinAcquisition::~BrillouinAcquisition() {
	CameraThread.exit();
	delete andor;
	qInfo(logInfo()) << "BrillouinAcquisition closed.";
	delete ui;
}

void BrillouinAcquisition::createCameraImage() {
	// QCustomPlotsTest
	// configure axis rect:

	QCustomPlot *customPlot = ui->customplot;

	customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom); // this will also allow rescaling the color scale by dragging/zooming
	customPlot->axisRect()->setupFullAxesBox(true);
	customPlot->xAxis->setLabel("x");
	customPlot->yAxis->setLabel("y");

	// this are the selection modes
	customPlot->setSelectionRectMode(QCP::srmZoom);	// allows to select region by rectangle
	customPlot->setSelectionRectMode(QCP::srmNone);	// allows to drag the position
	// the different modes need to be set on keypress (e.g. on shift key)

	// set background color to default light gray
	customPlot->setBackground(QColor(240, 240, 240, 255));
	customPlot->axisRect()->setBackground(Qt::white);

	// set up the QCPColorMap:
	colorMap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);
	int nx = 200;
	int ny = 200;
	colorMap->data()->setSize(nx, ny); // we want the color map to have nx * ny data points
	colorMap->data()->setRange(QCPRange(-4, 4), QCPRange(-4, 4)); // and span the coordinate range -4..4 in both key (x) and value (y) dimensions
																  // now we assign some data, by accessing the QCPColorMapData instance of the color map:
	double x, y;
	unsigned short z;
	//double z;
	for (int xIndex = 0; xIndex<nx; ++xIndex) {
		for (int yIndex = 0; yIndex<ny; ++yIndex) {
			colorMap->data()->cellToCoord(xIndex, yIndex, &x, &y);
			double r = 3 * qSqrt(x*x + y*y) + 1e-2;
			double tmp = x*(qCos(r + 2) / r - qSin(r + 2) / r); // the B field strength of dipole radiation (modulo physical constants)
			tmp += 0.5;
			tmp *= 65536;
			z = tmp;
			colorMap->data()->setCell(xIndex, yIndex, z);
		}
	}

	// turn off interpolation
	colorMap->setInterpolate(false);

	// add a color scale:
	QCPColorScale *colorScale = new QCPColorScale(customPlot);
	customPlot->plotLayout()->addElement(0, 1, colorScale); // add it to the right of the main axis rect
	colorScale->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
	colorMap->setColorScale(colorScale); // associate the color map with the color scale
	colorScale->axis()->setLabel("Intensity");

	// set the color gradient of the color map to one of the presets:
	colorMap->setGradient(QCPColorGradient::gpPolar);
	// we could have also created a QCPColorGradient instance and added own colors to
	// the gradient, see the documentation of QCPColorGradient for what's possible.

	// rescale the data dimension (color) such that all data points lie in the span visualized by the color gradient:
	colorMap->rescaleDataRange();

	// make sure the axis rect and color scale synchronize their bottom and top margins (so they line up):
	QCPMarginGroup *marginGroup = new QCPMarginGroup(customPlot);
	customPlot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
	colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);

	// rescale the key (x) and value (y) axes so the whole color map is visible:
	customPlot->rescaleAxes();

	// equal axis spacing
	//customPlot->xAxis->setRange(-1.5, 1.5);
	//customPlot->yAxis->setRange(-2.5, 2.5);
	//customPlot->yAxis->setScaleRatio(customPlot->xAxis, 1.0);
	//customPlot->replot();
	//customPlot->savePng("./export.png");
}

void BrillouinAcquisition::xAxisRangeChanged(const QCPRange &newRange) {
	// checks for certain range
	ui->customplot->xAxis->setRange(newRange.bounded(0, 3000));
}

void BrillouinAcquisition::yAxisRangeChanged(const QCPRange &newRange) {
	// checks for certain range
	ui->customplot->yAxis->setRange(newRange.bounded(0, 3000));
}

void BrillouinAcquisition::onNewImage(unsigned short* unpackedBuffer, AT_64 width, AT_64 height) {

	colorMap->data()->setSize(width, height); // we want the color map to have nx * ny data points
	colorMap->data()->setRange(QCPRange(0, width), QCPRange(0, height)); // and span the coordinate range -4..4 in both key (x) and value (y) dimensions

	double x, y;
	int tIndex;
	for (int xIndex = 0; xIndex<width; ++xIndex) {
		for (int yIndex = 0; yIndex<height; ++yIndex) {
			colorMap->data()->cellToCoord(xIndex, yIndex, &x, &y);
			tIndex = xIndex * height + yIndex;
			colorMap->data()->setCell(xIndex, yIndex, unpackedBuffer[tIndex]);
		}
	}
	colorMap->rescaleDataRange();
	ui->customplot->rescaleAxes();
	ui->customplot->replot();
}

void BrillouinAcquisition::on_actionConnect_Camera_triggered() {
	if (andor->getConnectionStatus()) {
		andor->disconnect();
		ui->actionConnect_Camera->setText("Connect Camera");
		ui->actionEnable_Cooling->setText("Enable Cooling");
		QIcon icon(":/BrillouinAcquisition/assets/00disconnected.png");
		ui->settingsWidget->setTabIcon(0, icon);
		ui->actionEnable_Cooling->setEnabled(FALSE);
	} else {
		andor->connect();
		ui->actionConnect_Camera->setText("Disconnect Camera");
		QIcon icon(":/BrillouinAcquisition/assets/01standby.png");
		ui->settingsWidget->setTabIcon(0, icon);
		ui->actionEnable_Cooling->setEnabled(TRUE);
	}
}

void BrillouinAcquisition::on_actionEnable_Cooling_triggered() {
	if (andor->getConnectionStatus()) {
		if (andor->getSensorCooling()) {
			andor->setSensorCooling(FALSE);
			ui->actionEnable_Cooling->setText("Enable Cooling");
			QIcon icon(":/BrillouinAcquisition/assets/01standby.png");
			ui->settingsWidget->setTabIcon(0, icon);
		}
		else {
			andor->setSensorCooling(TRUE);
			ui->actionEnable_Cooling->setText("Disable Cooling");
			QIcon icon(":/BrillouinAcquisition/assets/02cooling.png");
			ui->settingsWidget->setTabIcon(0, icon);
		}
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

void BrillouinAcquisition::on_camera_playPause_clicked() {
	// example for synchronous/blocking execution
	andor->acquireStartStop();
}

void BrillouinAcquisition::on_camera_singleShot_clicked() {
	// example for asynchronous/non-blocking execution
	QMetaObject::invokeMethod(andor, "acquireSingle", Qt::QueuedConnection);
}