#include "stdafx.h"
#include "BrillouinAcquisition.h"
#include "version.h"
#include "logger.h"
#include "simplemath.h"

BrillouinAcquisition::BrillouinAcquisition(QWidget *parent) noexcept :
	QMainWindow(parent), ui(new Ui::BrillouinAcquisitionClass) {
	ui->setupUi(this);

	// slot camera connection
	static QMetaObject::Connection connection;
	connection = QWidget::connect(
		m_andor,
		&Andor::s_previewBufferSettingsChanged,
		this,
		[this] { updatePlotLimits(m_BrillouinPlot, m_andor->previewBuffer->m_bufferSettings.roi); }
	);

	connection = QWidget::connect(
		m_andor,
		SIGNAL(cameraConnected(bool)),
		this,
		SLOT(cameraConnectionChanged(bool))
	);

	connection = QWidget::connect(
		m_andor,
		SIGNAL(noCameraFound()),
		this,
		SLOT(showNoCameraFound())
	);

	connection = QWidget::connect(
		m_andor,
		SIGNAL(cameraCoolingChanged(bool)),
		this,
		SLOT(cameraCoolingChanged(bool))
	);

	connection = QWidget::connect(
		m_andor,
		SIGNAL(s_previewRunning(bool)),
		this,
		SLOT(showPreviewRunning(bool))
	);

	connection = QWidget::connect(
		m_andor,
		SIGNAL(s_measurementRunning(bool)),
		this,
		SLOT(startPreview(bool))
	);

	connection = QWidget::connect(
		m_andor,
		SIGNAL(optionsChanged(CAMERA_OPTIONS)),
		this,
		SLOT(cameraOptionsChanged(CAMERA_OPTIONS))
	);

	connection = QWidget::connect(
		m_andor,
		SIGNAL(settingsChanged(CAMERA_SETTINGS)),
		this,
		SLOT(cameraSettingsChanged(CAMERA_SETTINGS))
	);

	connection = QWidget::connect(
		m_andor,
		SIGNAL(s_sensorTemperatureChanged(SensorTemperature)),
		this,
		SLOT(sensorTemperatureChanged(SensorTemperature))
	);

	// slot to limit the axis of the camera display after user interaction
	connection = QWidget::connect(
		ui->customplot->xAxis,
		SIGNAL(rangeChanged(QCPRange)),
		this,
		SLOT(xAxisRangeChanged(QCPRange))
	);
	connection = QWidget::connect(
		ui->customplot->yAxis,
		SIGNAL(rangeChanged(QCPRange)),
		this,
		SLOT(yAxisRangeChanged(QCPRange))
	);

	// slot to show current acquisition progress
	connection = QWidget::connect(
		m_acquisition,
		SIGNAL(s_acqRunning(bool)),
		this,
		SLOT(showAcqRunning(bool))
	);

	// slot to show current acquisition position
	connection = QWidget::connect(
		m_acquisition,
		SIGNAL(s_acqPosition(POINT3, int)),
		this,
		SLOT(showAcqPosition(POINT3, int))
	);

	// slot to show current acquisition progress
	connection = QWidget::connect(
		m_acquisition,
		SIGNAL(s_acqProgress(int, double, int)),
		this,
		SLOT(showAcqProgress(int, double, int))
	);

	// slot to update filename
	connection = QWidget::connect(
		m_acquisition,
		SIGNAL(s_filenameChanged(std::string)),
		this,
		SLOT(updateFilename(std::string))
	);

	// slot to show calibration running
	connection = QWidget::connect(
		m_acquisition,
		SIGNAL(s_acqCalibrationRunning(bool)),
		this,
		SLOT(showCalibrationRunning(bool))
	);

	// slot to show time until next calibration
	connection = QWidget::connect(
		m_acquisition,
		SIGNAL(s_acqTimeToCalibration(int)),
		this,
		SLOT(showCalibrationInterval(int))
	);

	// slot to show repetitions
	connection = QWidget::connect(
		m_acquisition,
		SIGNAL(s_acqRepetitionProgress(int, int)),
		this,
		SLOT(showRepProgress(int, int))
	);

	qRegisterMetaType<std::string>("std::string");
	qRegisterMetaType<AT_64>("AT_64");
	qRegisterMetaType<ACQUISITION_SETTINGS>("ACQUISITION_SETTINGS");
	qRegisterMetaType<CAMERA_SETTINGS>("ACQUISITION_SETTINGS");
	qRegisterMetaType<CAMERA_OPTIONS>("CAMERA_OPTIONS");
	qRegisterMetaType<std::vector<int>>("std::vector<int>");
	qRegisterMetaType<QSerialPort::SerialPortError>("QSerialPort::SerialPortError");
	qRegisterMetaType<IMAGE*>("IMAGE*");
	qRegisterMetaType<CALIBRATION*>("CALIBRATION*");
	qRegisterMetaType<ScanControl::SCAN_PRESET>("ScanControl::SCAN_PRESET");
	qRegisterMetaType<DeviceElement>("DeviceElement");
	qRegisterMetaType<SensorTemperature>("SensorTemperature");
	qRegisterMetaType<POINT3>("POINT3");
	qRegisterMetaType<std::vector<POINT3>>("std::vector<POINT3>");
	qRegisterMetaType<BOUNDS>("BOUNDS");
	qRegisterMetaType<QMouseEvent*>("QMouseEvent*");
	

	QIcon icon(":/BrillouinAcquisition/assets/00disconnected.png");
	ui->settingsWidget->setTabIcon(0, icon);
	ui->settingsWidget->setTabIcon(1, icon);
	ui->settingsWidget->setTabIcon(2, icon);
	ui->settingsWidget->setTabIcon(3, icon);
	ui->settingsWidget->setIconSize(QSize(16, 16));

	ui->actionEnable_Cooling->setEnabled(false);
	ui->autoscalePlot->setChecked(m_BrillouinPlot.autoscale);

	initScanControl();
	initCamera();
	// start camera thread
	m_acquisitionThread.startWorker(m_andor);
	// start acquisition thread
	m_acquisitionThread.startWorker(m_acquisition);

	// set up the QCPColorMap:
	m_BrillouinPlot = {
		ui->customplot,
		new QCPColorMap(ui->customplot->xAxis, ui->customplot->yAxis),
		{ 100, 300 },
		false,
		gpParula
	};
	m_ODTPlot = {
		ui->customplot_brightfield,
		new QCPColorMap(ui->customplot_brightfield->xAxis, ui->customplot_brightfield->yAxis),
		{ 0, 100 },
		false,
		gpGrayscale
	};

	// set up the camera image plot
	BrillouinAcquisition::initializePlot(m_BrillouinPlot);
	BrillouinAcquisition::initializePlot(m_ODTPlot);

	connect(m_ODTPlot.plotHandle, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(plotClick(QMouseEvent*)));

	updateAcquisitionSettings();
	initSettingsDialog();

	// Set up GUI
	initBeampathButtons();
	updateSavedPositions();

	// disable keyboard tracking on stage position input
	// so only complete numbers emit signals
	ui->setPositionX->setKeyboardTracking(false);
	ui->setPositionY->setKeyboardTracking(false);
	ui->setPositionZ->setKeyboardTracking(false);

	ui->parametersWidget->layout()->setAlignment(Qt::AlignTop);

	ui->brightfieldImage->hide();
}

BrillouinAcquisition::~BrillouinAcquisition() {
	//m_cameraThread.exit();
	//m_cameraThread.wait();
	//m_microscopeThread.exit();
	//m_microscopeThread.wait();
	m_acquisitionThread.exit();
	m_acquisitionThread.wait();
	delete m_andor;
	delete m_pointGrey;
	qInfo(logInfo()) << "BrillouinAcquisition closed.";
	delete ui;
}

void BrillouinAcquisition::plotClick(QMouseEvent* event) {
	QPoint position = event->pos();

	double posX = m_ODTPlot.plotHandle->xAxis->pixelToCoord(position.x());
	double posY = m_ODTPlot.plotHandle->yAxis->pixelToCoord(position.y());

	// TODO: Set laser focus to this position
}

void BrillouinAcquisition::showEvent(QShowEvent* event) {
	QWidget::showEvent(event);

	// connect camera and microscope automatically
	QMetaObject::invokeMethod(m_andor, "connectDevice", Qt::QueuedConnection);
	QMetaObject::invokeMethod(m_scanControl, "connectDevice", Qt::QueuedConnection);
}

void BrillouinAcquisition::setElement(DeviceElement element, int position) {
	QMetaObject::invokeMethod(m_scanControl, "setElement", Qt::QueuedConnection, Q_ARG(DeviceElement, element), Q_ARG(int, position));
}

void BrillouinAcquisition::on_autoscalePlot_stateChanged(int state) {
	m_BrillouinPlot.autoscale = (bool)state;
}

void BrillouinAcquisition::on_autoscalePlot_brightfield_stateChanged(int state) {
	m_ODTPlot.autoscale = (bool)state;
}

void BrillouinAcquisition::setPreset(ScanControl::SCAN_PRESET preset) {
	QMetaObject::invokeMethod(m_scanControl, "setElements", Qt::QueuedConnection, Q_ARG(ScanControl::SCAN_PRESET, preset));
}

void BrillouinAcquisition::cameraOptionsChanged(CAMERA_OPTIONS options) {
	m_cameraOptions.ROIHeightLimits = options.ROIHeightLimits;
	m_cameraOptions.ROIWidthLimits = options.ROIWidthLimits;

	// set size of colormap to maximum image size
	m_BrillouinPlot.colorMap->data()->setSize(options.ROIWidthLimits[1], options.ROIHeightLimits[1]);

	addListToComboBox(ui->triggerMode, options.triggerModes);
	addListToComboBox(ui->binning, options.imageBinnings);
	addListToComboBox(ui->pixelReadoutRate, options.pixelReadoutRates);
	addListToComboBox(ui->cycleMode, options.cycleModes);
	addListToComboBox(ui->preAmpGain, options.preAmpGains);
	addListToComboBox(ui->pixelEncoding, options.pixelEncodings);

	ui->exposureTime->setMinimum(options.exposureTimeLimits[0]);
	ui->exposureTime->setMaximum(options.exposureTimeLimits[1]);
	ui->frameCount->setMinimum(options.frameCountLimits[0]);
	ui->frameCount->setMaximum(options.frameCountLimits[1]);

	ui->ROIHeight->setMinimum(options.ROIHeightLimits[0]);
	ui->ROIHeight->setMaximum(options.ROIHeightLimits[1]);
	ui->ROIHeight->setValue(options.ROIHeightLimits[1]);
	ui->ROITop->setMinimum(1);
	ui->ROITop->setMaximum(options.ROIHeightLimits[1]);
	ui->ROITop->setValue(1);
	ui->ROIWidth->setMinimum(options.ROIWidthLimits[0]);
	ui->ROIWidth->setMaximum(options.ROIWidthLimits[1]);
	ui->ROIWidth->setValue(options.ROIWidthLimits[1]);
	ui->ROILeft->setMinimum(1);
	ui->ROILeft->setMaximum(options.ROIWidthLimits[1]);
	ui->ROILeft->setValue(1);
}

void BrillouinAcquisition::showAcqPosition(POINT3 position, int imageNr) {
	showPosition(position);
	ui->imageNr->setText(QString::number(imageNr));
}

void BrillouinAcquisition::showPosition(POINT3 position) {
	ui->positionX->setText(QString::number(position.x));
	ui->positionY->setText(QString::number(position.y));
	ui->positionZ->setText(QString::number(position.z));
	if (!ui->setPositionX->hasFocus()) {
		ui->setPositionX->blockSignals(true);
		ui->setPositionX->setValue(position.x);
		ui->setPositionX->blockSignals(false);
	}
	if (!ui->setPositionY->hasFocus()) {
		ui->setPositionY->blockSignals(true);
		ui->setPositionY->setValue(position.y);
		ui->setPositionY->blockSignals(false);
	}
	if (!ui->setPositionZ->hasFocus()) {
		ui->setPositionZ->blockSignals(true);
		ui->setPositionZ->setValue(position.z);
		ui->setPositionZ->blockSignals(false);
	}
}

void BrillouinAcquisition::setHomePositionBounds(BOUNDS bounds) {
	// set limits on manual stage control
	ui->setPositionX->setMinimum(bounds.xMin);
	ui->setPositionX->setMaximum(bounds.xMax);
	ui->setPositionY->setMinimum(bounds.yMin);
	ui->setPositionY->setMaximum(bounds.yMax);
	ui->setPositionZ->setMinimum(bounds.zMin);
	ui->setPositionZ->setMaximum(bounds.zMax);
}

void BrillouinAcquisition::setCurrentPositionBounds(BOUNDS bounds) {
	// set limits on AOI control
	//x
	ui->startX->setMinimum(bounds.xMin);
	ui->startX->setMaximum(bounds.xMax);
	ui->endX->setMinimum(bounds.xMin);
	ui->endX->setMaximum(bounds.xMax);

	//y
	ui->startY->setMinimum(bounds.yMin);
	ui->startY->setMaximum(bounds.yMax);
	ui->endY->setMinimum(bounds.yMin);
	ui->endY->setMaximum(bounds.yMax);

	//z
	ui->startZ->setMinimum(bounds.zMin);
	ui->startZ->setMaximum(bounds.zMax);
	ui->endZ->setMinimum(bounds.zMin);
	ui->endZ->setMaximum(bounds.zMax);
}

void BrillouinAcquisition::showAcqProgress(int state, double progress, int seconds) {
	ui->progressBar->setValue(progress);

	QString string;
	if (state == ACQUISITION_STATES::ABORTED) {
		string = "Acquisition aborted.";
	} else if (state == ACQUISITION_STATES::STARTED) {
		string = "Acquisition started.";
	} else if (state == ACQUISITION_STATES::FINISHED) {
		string = "Acquisition finished.";
	} else {
		QString timeString = formatSeconds(seconds);
		string.sprintf("%02.1f %% finished, ", progress);
		string += timeString;
		string += " remaining.";
	}
	ui->progressBar->setFormat(string);
}

QString BrillouinAcquisition::formatSeconds(int seconds) {
	QString string;
	if (seconds > 3600) {
		int hours = floor((double)seconds / 3600);
		int minutes = floor((seconds - hours * 3600) / 60);
		string.sprintf("%02.0f:%02.0f hours", (double)hours, (double)minutes);
	} else if (seconds > 60) {
		int minutes = floor(seconds / 60);
		seconds = floor(seconds - minutes * 60);
		string.sprintf("%02.0f:%02.0f minutes", (double)minutes, (double)seconds);
	} else {
		string.sprintf("%2.0f seconds", (double)seconds);
	}
	return string;
}

void BrillouinAcquisition::on_actionQuit_triggered() {
	QApplication::quit();
}

void BrillouinAcquisition::showCalibrationInterval(int value) {
	ui->calibrationProgress->setValue(value);
	ui->calibrationProgress->setFormat("Time to next calibration.");
}

void BrillouinAcquisition::showCalibrationRunning(bool isCalibrating) {
	if (isCalibrating) {
		ui->calibrationProgress->setValue(100);
		ui->calibrationProgress->setFormat("Acquiring calibration.");
	}
}

void BrillouinAcquisition::addListToComboBox(QComboBox* box, std::vector<AT_WC*> list, bool clear) {
	if (clear) {
		box->clear();
	}
	std::for_each(list.begin(), list.end(), [box](AT_WC* &item) {
		box->addItem(QString::fromWCharArray(item));
	});
};

void BrillouinAcquisition::cameraSettingsChanged(CAMERA_SETTINGS settings) {
	ui->exposureTime->setValue(settings.exposureTime);
	ui->frameCount->setValue(settings.frameCount);
	//ui->ROILeft->setValue(settings.roi.left);
	//ui->ROIWidth->setValue(settings.roi.width);
	//ui->ROITop->setValue(settings.roi.top);
	//ui->ROIHeight->setValue(settings.roi.height);

	ui->triggerMode->setCurrentText(QString::fromWCharArray(settings.readout.triggerMode));
	ui->binning->setCurrentText(QString::fromWCharArray(settings.roi.binning));

	ui->pixelReadoutRate->setCurrentText(QString::fromWCharArray(settings.readout.pixelReadoutRate));
	ui->cycleMode->setCurrentText(QString::fromWCharArray(settings.readout.cycleMode));
	ui->preAmpGain->setCurrentText(QString::fromWCharArray(settings.readout.preAmpGain));
	ui->pixelEncoding->setCurrentText(QString::fromWCharArray(settings.readout.pixelEncoding));
}

void BrillouinAcquisition::sensorTemperatureChanged(SensorTemperature sensorTemperature) {
	ui->sensorTemp->setValue(sensorTemperature.temperature);
	QIcon icon;
	if (sensorTemperature.status == COOLER_OFF || sensorTemperature.status == FAULT || sensorTemperature.status == DRIFT) {
		icon = QIcon(":/BrillouinAcquisition/assets/01standby.png");
	} else if (sensorTemperature.status == COOLING || sensorTemperature.status == NOT_STABILISED) {
		icon = QIcon(":/BrillouinAcquisition/assets/02cooling.png");
	} else if (sensorTemperature.status == STABILISED) {
		icon = QIcon(":/BrillouinAcquisition/assets/03ready.png");
	} else {
		icon = QIcon(":/BrillouinAcquisition/assets/00disconnected.png");
	}
	ui->settingsWidget->setTabIcon(0, icon);
}

void BrillouinAcquisition::initializePlot(PLOT_SETTINGS plotSettings) {
	// configure axis rect

	plotSettings.plotHandle->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom); // this will also allow rescaling the color scale by dragging/zooming
	plotSettings.plotHandle->axisRect()->setupFullAxesBox(true);
	//customPlot->xAxis->setLabel("x");
	//customPlot->yAxis->setLabel("y");

	// this are the selection modes
	plotSettings.plotHandle->setSelectionRectMode(QCP::srmZoom);	// allows to select region by rectangle
	plotSettings.plotHandle->setSelectionRectMode(QCP::srmNone);	// allows to drag the position

	// set background color to default light gray
	plotSettings.plotHandle->setBackground(QColor(240, 240, 240, 255));
	plotSettings.plotHandle->axisRect()->setBackground(Qt::white);

	// fill map with zero
	plotSettings.colorMap->data()->fill(0);

	// turn off interpolation
	plotSettings.colorMap->setInterpolate(false);

	// add a color scale:
	QCPColorScale *colorScale = new QCPColorScale(plotSettings.plotHandle);
	plotSettings.plotHandle->plotLayout()->addElement(0, 1, colorScale); // add it to the right of the main axis rect
	colorScale->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
	plotSettings.colorMap->setColorScale(colorScale); // associate the color map with the color scale
	colorScale->axis()->setLabel("Intensity");

	// set the color gradient of the color map to one of the presets:
	QCPColorGradient gradient = QCPColorGradient();
	setColormap(&gradient, plotSettings.gradient);
	plotSettings.colorMap->setGradient(gradient);

	plotSettings.colorMap->setDataRange(plotSettings.cLim);
	// rescale the data dimension (color) such that all data points lie in the span visualized by the color gradient:
	if (plotSettings.autoscale) {
		plotSettings.colorMap->rescaleDataRange();
	}

	// make sure the axis rect and color scale synchronize their bottom and top margins (so they line up):
	QCPMarginGroup *marginGroup = new QCPMarginGroup(plotSettings.plotHandle);
	plotSettings.plotHandle->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
	colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);

	// rescale the key (x) and value (y) axes so the whole color map is visible:
	plotSettings.plotHandle->rescaleAxes();
}

void BrillouinAcquisition::xAxisRangeChanged(const QCPRange &newRange) {
	ui->customplot->xAxis->setRange(newRange.bounded(1, m_cameraOptions.ROIWidthLimits[1]));
	m_deviceSettings.camera.roi.left = newRange.lower;
	m_deviceSettings.camera.roi.width = newRange.upper - newRange.lower + 1;
	settingsCameraUpdate(ROI_SOURCE::PLOT);
}

void BrillouinAcquisition::yAxisRangeChanged(const QCPRange &newRange) {
	ui->customplot->yAxis->setRange(newRange.bounded(1, m_cameraOptions.ROIHeightLimits[1]));
	m_deviceSettings.camera.roi.top = newRange.lower;
	m_deviceSettings.camera.roi.height = newRange.upper - newRange.lower + 1;
	settingsCameraUpdate(ROI_SOURCE::PLOT);
}

void BrillouinAcquisition::on_ROILeft_valueChanged(int left) {
	m_deviceSettings.camera.roi.left = left;
	settingsCameraUpdate(ROI_SOURCE::BOX);
}

void BrillouinAcquisition::on_ROIWidth_valueChanged(int width) {
	m_deviceSettings.camera.roi.width = width;
	settingsCameraUpdate(ROI_SOURCE::BOX);
}

void BrillouinAcquisition::on_ROITop_valueChanged(int top) {
	m_deviceSettings.camera.roi.top = top;
	settingsCameraUpdate(ROI_SOURCE::BOX);
}

void BrillouinAcquisition::on_ROIHeight_valueChanged(int height) {
	m_deviceSettings.camera.roi.height = height;
	settingsCameraUpdate(ROI_SOURCE::BOX);
}

void BrillouinAcquisition::settingsCameraUpdate(int source) {
	// check that values are valid
	// start must be >= than 1 and <= than (imageSize - minROIsize + 1)
	// ROIsize must be within [minROIsize, maxROIsize]
	// end must be >= than <= than the size of the image

	// check these requirements
	// for x
	std::vector<AT_64> xIn = { m_deviceSettings.camera.roi.left, m_deviceSettings.camera.roi.width };
	std::vector<AT_64> xOut = checkROI(xIn, m_cameraOptions.ROIWidthLimits);
	bool xChanged = (xIn != xOut);
	if (xChanged) {
		m_deviceSettings.camera.roi.left = xOut[0];
		m_deviceSettings.camera.roi.width = xOut[1];
	}

	//for y
	std::vector<AT_64> yIn = { m_deviceSettings.camera.roi.top, m_deviceSettings.camera.roi.height };
	std::vector<AT_64> yOut = checkROI(yIn, m_cameraOptions.ROIHeightLimits);
	bool yChanged = (yIn != yOut);
	if (yChanged) {
		m_deviceSettings.camera.roi.top = yOut[0];
		m_deviceSettings.camera.roi.height = yOut[1];
	}

	// set values of manual input fields
	if (source == ROI_SOURCE::PLOT || xChanged) {
		// dont retrigger a round of checking
		ui->ROILeft->blockSignals(true);
		ui->ROIWidth->blockSignals(true);
		ui->ROILeft->setValue(m_deviceSettings.camera.roi.left);
		ui->ROIWidth->setValue(m_deviceSettings.camera.roi.width);
		ui->ROILeft->blockSignals(false);
		ui->ROIWidth->blockSignals(false);
	}
	if (source == ROI_SOURCE::PLOT || yChanged) {
		// dont retrigger a round of checking
		ui->ROITop->blockSignals(true);
		ui->ROIHeight->blockSignals(true);
		ui->ROITop->setValue(m_deviceSettings.camera.roi.top);
		ui->ROIHeight->setValue(m_deviceSettings.camera.roi.height);
		ui->ROITop->blockSignals(false);
		ui->ROIHeight->blockSignals(false);
	}
	// set plot range
	//ui->customplot->yAxis->setRange(newRange.bounded(1, m_cameraOptions.ROIHeightLimits[1]));
	if (source == ROI_SOURCE::BOX || xChanged) {
		ui->customplot->xAxis->setRange(QCPRange(m_deviceSettings.camera.roi.left, m_deviceSettings.camera.roi.left + m_deviceSettings.camera.roi.width - 1));
	}
	if (source == ROI_SOURCE::BOX || yChanged) {
		ui->customplot->yAxis->setRange(QCPRange(m_deviceSettings.camera.roi.top, m_deviceSettings.camera.roi.top + m_deviceSettings.camera.roi.height - 1));
	}
	if (source == ROI_SOURCE::BOX || xChanged || yChanged) {
		ui->customplot->replot();
	}
}

std::vector<AT_64> BrillouinAcquisition::checkROI(std::vector<AT_64> values, std::vector<AT_64> requirements) {
	// check that lower value is in valid range
	if (values[0] < 1) {
	// must be larger than 0, counting camera pixels starts at 1
		values[0] = 1;
	} else if (values[0] > (requirements[1] - requirements[0] + 1)) {
	// must be equal to or smaller than the image size minus the minimal ROI size + 1
		values[0] = requirements[1] - requirements[0] + 1;
	}
	// check that ROI size is valid
	if (values[1] < requirements[0]) {
	// size must at least be equal to the minmal allowed ROI size
		values[1] = requirements[0];
	} else if (values[1] > (requirements[1] - values[0] + 1)) {
		values[1] = requirements[1] - values[0] + 1;
	}
	return values;
}

void BrillouinAcquisition::updatePlotLimits(PLOT_SETTINGS plotSettings, CAMERA_ROI roi) {
	// set the properties of the colormap to the correct values of the preview buffer
	plotSettings.colorMap->data()->setSize(roi.width, roi.height);
	plotSettings.colorMap->data()->setRange(
		QCPRange(roi.left, roi.width + roi.left - 1),
		QCPRange(roi.top, roi.height + roi.top - 1)
	);
}

void BrillouinAcquisition::showPreviewRunning(bool isRunning) {
	if (isRunning) {
		ui->camera_playPause->setText("Stop");
	} else {
		ui->camera_playPause->setText("Play");
	}
	startPreview(isRunning);
}

void BrillouinAcquisition::showBrightfieldPreviewRunning(bool isRunning) {
	if (isRunning) {
		ui->camera_playPause_brightfield->setText("Stop");
	} else {
		ui->camera_playPause_brightfield->setText("Play");
	}
	startBrightfieldPreview(isRunning);
}

void BrillouinAcquisition::showAcqRunning(bool isRunning) {
	if (isRunning) {
		ui->acquisitionStart->setText("Stop");
	} else {
		ui->acquisitionStart->setText("Start");
	}
	m_measurementRunning = isRunning;
	ui->startX->setEnabled(!m_measurementRunning);
	ui->startY->setEnabled(!m_measurementRunning);
	ui->startZ->setEnabled(!m_measurementRunning);
	ui->endX->setEnabled(!m_measurementRunning);
	ui->endY->setEnabled(!m_measurementRunning);
	ui->endZ->setEnabled(!m_measurementRunning);
	ui->stepsX->setEnabled(!m_measurementRunning);
	ui->stepsY->setEnabled(!m_measurementRunning);
	ui->stepsZ->setEnabled(!m_measurementRunning);
	ui->camera_playPause->setEnabled(!m_measurementRunning);
	ui->camera_singleShot->setEnabled(!m_measurementRunning);
	ui->setHome->setEnabled(!m_measurementRunning);
	ui->setPositionX->setEnabled(!m_measurementRunning);
	ui->setPositionY->setEnabled(!m_measurementRunning);
	ui->setPositionZ->setEnabled(!m_measurementRunning);
}

void BrillouinAcquisition::startPreview(bool isRunning) {
	// if preview was not running, start it, else leave it running (don't start it twice)
	if (!m_previewRunning && isRunning) {
		m_previewRunning = true;
		onNewImage();
	}
	m_previewRunning = isRunning;
}

void BrillouinAcquisition::startBrightfieldPreview(bool isRunning) {
	// if preview was not running, start it, else leave it running (don't start it twice)
	if (!m_brightfieldPreviewRunning && isRunning) {
		m_brightfieldPreviewRunning = true;
		onNewBrightfieldImage();
	}
	m_brightfieldPreviewRunning = isRunning;
}

void BrillouinAcquisition::onNewImage() {
	if (m_previewRunning) {
		PLOT_SETTINGS plotSettings = m_BrillouinPlot;
		{
			std::lock_guard<std::mutex> lockGuard(m_andor->previewBuffer->m_mutex);
			// if no image is ready return immediately
			if (!m_andor->previewBuffer->m_buffer->m_usedBuffers->tryAcquire()) {
				QMetaObject::invokeMethod(this, "onNewImage", Qt::QueuedConnection);
				return;
			}

			auto unpackedBuffer = reinterpret_cast<unsigned short*>(m_andor->previewBuffer->m_buffer->getReadBuffer());

			int tIndex;
			for (gsl::index xIndex = 0; xIndex < m_andor->previewBuffer->m_bufferSettings.roi.height; ++xIndex) {
				for (gsl::index yIndex = 0; yIndex < m_andor->previewBuffer->m_bufferSettings.roi.width; ++yIndex) {
					tIndex = xIndex * m_andor->previewBuffer->m_bufferSettings.roi.width + yIndex;
					plotSettings.colorMap->data()->setCell(yIndex, xIndex, unpackedBuffer[tIndex]);
				}
			}

		}
		m_andor->previewBuffer->m_buffer->m_freeBuffers->release();
		if (plotSettings.autoscale) {
			plotSettings.colorMap->rescaleDataRange();
		}
		plotSettings.plotHandle->replot();
		
		QMetaObject::invokeMethod(this, "onNewImage", Qt::QueuedConnection);
	}
}

void BrillouinAcquisition::onNewBrightfieldImage() {
	if (m_brightfieldPreviewRunning) {
		PLOT_SETTINGS plotSettings = m_ODTPlot;
		{
			std::lock_guard<std::mutex> lockGuard(m_pointGrey->previewBuffer->m_mutex);
			// if no image is ready return immediately
			if (!m_pointGrey->previewBuffer->m_buffer->m_usedBuffers->tryAcquire()) {
				QMetaObject::invokeMethod(this, "onNewBrightfieldImage", Qt::QueuedConnection);
				return;
			}

			auto unpackedBuffer = m_pointGrey->previewBuffer->m_buffer->getReadBuffer();

			int tIndex;
			for (gsl::index xIndex = 0; xIndex < m_pointGrey->previewBuffer->m_bufferSettings.roi.height; ++xIndex) {
				for (gsl::index yIndex = 0; yIndex < m_pointGrey->previewBuffer->m_bufferSettings.roi.width; ++yIndex) {
					tIndex = xIndex * m_pointGrey->previewBuffer->m_bufferSettings.roi.width + yIndex;
					plotSettings.colorMap->data()->setCell(yIndex, xIndex, unpackedBuffer[tIndex]);
				}
			}

		}
		m_pointGrey->previewBuffer->m_buffer->m_freeBuffers->release();
		if (plotSettings.autoscale) {
			plotSettings.colorMap->rescaleDataRange();
		}
		plotSettings.plotHandle->replot();

		QMetaObject::invokeMethod(this, "onNewBrightfieldImage", Qt::QueuedConnection);
	}
}

void BrillouinAcquisition::on_actionConnect_Camera_triggered() {
	if (m_andor->getConnectionStatus()) {
		QMetaObject::invokeMethod(m_andor, "disconnectDevice", Qt::QueuedConnection);
	} else {
		QMetaObject::invokeMethod(m_andor, "connectDevice", Qt::QueuedConnection);
	}
}

void BrillouinAcquisition::cameraConnectionChanged(bool isConnected) {
	if (isConnected) {
		ui->actionConnect_Camera->setText("Disconnect Camera");
		QIcon icon(":/BrillouinAcquisition/assets/01standby.png");
		ui->settingsWidget->setTabIcon(0, icon);
		ui->actionEnable_Cooling->setEnabled(true);
		ui->camera_playPause->setEnabled(true);
		ui->camera_singleShot->setEnabled(true);
		// switch on cooling automatically
		QMetaObject::invokeMethod(m_andor, "setSensorCooling", Qt::QueuedConnection, Q_ARG(bool, true));
	} else {
		ui->actionConnect_Camera->setText("Connect Camera");
		ui->actionEnable_Cooling->setText("Enable Cooling");
		QIcon icon(":/BrillouinAcquisition/assets/00disconnected.png");
		ui->settingsWidget->setTabIcon(0, icon);
		ui->actionEnable_Cooling->setEnabled(false);
		ui->camera_playPause->setEnabled(false);
		ui->camera_singleShot->setEnabled(false);
	}
}

void BrillouinAcquisition::showNoCameraFound() {
	QMessageBox::critical(this, "Camera not found.", "No camera was found. Switch on the camera and restart the program.");
}

void BrillouinAcquisition::on_actionEnable_Cooling_triggered() {
	if (m_andor->getConnectionStatus()) {
		if (m_andor->getSensorCooling()) {
			QMetaObject::invokeMethod(m_andor, "setSensorCooling", Qt::QueuedConnection, Q_ARG(bool, false));
		} else {
			QMetaObject::invokeMethod(m_andor, "setSensorCooling", Qt::QueuedConnection, Q_ARG(bool, true));
		}
	}
}

void BrillouinAcquisition::cameraCoolingChanged(bool isCooling) {
	if (isCooling) {
		ui->actionEnable_Cooling->setText("Disable Cooling");
		QIcon icon(":/BrillouinAcquisition/assets/02cooling.png");
		ui->settingsWidget->setTabIcon(0, icon);
	} else {
		ui->actionEnable_Cooling->setText("Enable Cooling");
		QIcon icon(":/BrillouinAcquisition/assets/01standby.png");
		ui->settingsWidget->setTabIcon(0, icon);
	}
}

void BrillouinAcquisition::on_actionConnect_Stage_triggered() {
	if (m_scanControl->getConnectionStatus()) {
		QMetaObject::invokeMethod(m_scanControl, "disconnectDevice", Qt::QueuedConnection);
	} else {
		QMetaObject::invokeMethod(m_scanControl, "connectDevice", Qt::QueuedConnection);
	}
}

void BrillouinAcquisition::microscopeConnectionChanged(bool isConnected) {
	if (isConnected) {
		ui->actionConnect_Stage->setText("Disconnect Microscope");
		QIcon icon(":/BrillouinAcquisition/assets/03ready.png");
		ui->settingsWidget->setTabIcon(1, icon);
		ui->settingsWidget->setTabIcon(2, icon);
	} else {
		ui->actionConnect_Stage->setText("Connect Microscope");
		QIcon icon(":/BrillouinAcquisition/assets/00disconnected.png");
		ui->settingsWidget->setTabIcon(1, icon);
		ui->settingsWidget->setTabIcon(2, icon);
	}
}

void BrillouinAcquisition::on_actionConnect_Brightfield_camera_triggered() {
	if (m_pointGrey->getConnectionStatus()) {
		QMetaObject::invokeMethod(m_pointGrey, "disconnectDevice", Qt::QueuedConnection);
	} else {
		QMetaObject::invokeMethod(m_pointGrey, "connectDevice", Qt::QueuedConnection);
	}
}

void BrillouinAcquisition::brightfieldCameraConnectionChanged(bool isConnected) {
	if (isConnected) {
		ui->actionConnect_Brightfield_camera->setText("Disconnect Brightfield Camera");
		ui->brightfieldImage->show();
		ui->camera_playPause_brightfield->setEnabled(true);
		QIcon icon(":/BrillouinAcquisition/assets/03ready.png");
		ui->settingsWidget->setTabIcon(3, icon);
	}
	else {
		ui->actionConnect_Brightfield_camera->setText("Connect Brightfield Camera");
		ui->brightfieldImage->hide();
		ui->camera_playPause_brightfield->setEnabled(false);
		QIcon icon(":/BrillouinAcquisition/assets/00disconnected.png");
		ui->settingsWidget->setTabIcon(3, icon);
	}
}

void BrillouinAcquisition::on_camera_playPause_brightfield_clicked() {
	if (!m_pointGrey->m_isPreviewRunning) {
		QMetaObject::invokeMethod(m_pointGrey, "startPreview", Qt::QueuedConnection, Q_ARG(CAMERA_SETTINGS, m_acquisitionSettings.camera));
	}
	else {
		m_pointGrey->m_isPreviewRunning = false;
	}
}

void BrillouinAcquisition::on_actionSettings_Stage_triggered() {
	m_scanControlDropdown->setCurrentIndex((int)m_scanControllerType);
	m_settingsDialog->show();
}

void BrillouinAcquisition::saveSettings() {
	if (m_scanControllerType != m_scanControllerTypeTemporary) {
		m_scanControllerType = m_scanControllerTypeTemporary;
		initScanControl();
		initBeampathButtons();
	}
	if (m_cameraType != m_cameraTypeTemporary) {
		m_cameraType = m_cameraTypeTemporary;
		initCamera();
	}
	m_settingsDialog->hide();
}

void BrillouinAcquisition::cancelSettings() {
	m_scanControllerTypeTemporary = m_scanControllerType;
	m_cameraTypeTemporary = m_cameraType;
	m_settingsDialog->hide();
}

void BrillouinAcquisition::initSettingsDialog() {
	m_scanControllerTypeTemporary = m_scanControllerType;
	m_settingsDialog = new QDialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
	m_settingsDialog->setWindowTitle("Settings");
	m_settingsDialog->setWindowModality(Qt::ApplicationModal);

	QVBoxLayout *vLayout = new QVBoxLayout(m_settingsDialog);

	QWidget *daqWidget = new QWidget();
	daqWidget->setMinimumHeight(100);
	daqWidget->setMinimumWidth(400);
	QGroupBox *box = new QGroupBox(daqWidget);
	box->setTitle("Scanning device");
	box->setMinimumHeight(80);
	box->setMinimumWidth(400);

	vLayout->addWidget(daqWidget);

	QHBoxLayout *layout = new QHBoxLayout(box);

	QLabel *label = new QLabel("Currently selected device");
	layout->addWidget(label);

	m_scanControlDropdown = new QComboBox();
	layout->addWidget(m_scanControlDropdown);
	gsl::index i{ 0 };
	for (auto type : m_scanControl->SCAN_DEVICE_NAMES) {
		m_scanControlDropdown->insertItem(i, QString::fromStdString(type));
		i++;
	}
	m_scanControlDropdown->setCurrentIndex((int)m_scanControllerType);

	static QMetaObject::Connection connection = QWidget::connect(
		m_scanControlDropdown,
		SIGNAL(currentIndexChanged(int)),
		this,
		SLOT(selectScanningDevice(int))
	);

	/*
	 * Widget for ODT/Fluorescence camera selection
	 */
	QWidget *cameraWidget = new QWidget();
	cameraWidget->setMinimumHeight(100);
	cameraWidget->setMinimumWidth(400);
	QGroupBox *camBox = new QGroupBox(cameraWidget);
	camBox->setTitle("ODT/Fluorescence camera");
	camBox->setMinimumHeight(80);
	camBox->setMinimumWidth(400);

	vLayout->addWidget(cameraWidget);

	QHBoxLayout *camLayout = new QHBoxLayout(camBox);

	QLabel *camLabel = new QLabel("Currently selected camera");
	camLayout->addWidget(camLabel);

	m_cameraDropdown = new QComboBox();
	camLayout->addWidget(m_cameraDropdown);
	i = 0;
	for (auto type : CAMERA_DEVICE_NAMES) {
		m_cameraDropdown->insertItem(i, QString::fromStdString(type));
		i++;
	}
	m_cameraDropdown->setCurrentIndex((int)m_cameraType);

	connection = QWidget::connect(
		m_cameraDropdown,
		SIGNAL(currentIndexChanged(int)),
		this,
		SLOT(selectCameraDevice(int))
	);

	/*
	 * Ok and Cancel buttons
	 */
	QWidget *buttonWidget = new QWidget();
	vLayout->addWidget(buttonWidget);

	QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
	buttonLayout->setMargin(0);

	QPushButton *okButton = new QPushButton();
	okButton->setText(tr("OK"));
	okButton->setMaximumWidth(100);
	buttonLayout->addWidget(okButton);
	buttonLayout->setAlignment(okButton, Qt::AlignRight);

	connection = QWidget::connect(
		okButton,
		SIGNAL(clicked()),
		this,
		SLOT(saveSettings())
	);

	QPushButton *cancelButton = new QPushButton();
	cancelButton->setText(tr("Cancel"));
	cancelButton->setMaximumWidth(100);
	buttonLayout->addWidget(cancelButton);

	connection = QWidget::connect(
		cancelButton,
		SIGNAL(clicked()),
		this,
		SLOT(cancelSettings())
	);

	m_settingsDialog->layout()->setSizeConstraint(QLayout::SetFixedSize);
}

void BrillouinAcquisition::selectScanningDevice(int index) {
	m_scanControllerTypeTemporary = (ScanControl::SCAN_DEVICE)index;
}

void BrillouinAcquisition::selectCameraDevice(int index) {
	m_cameraTypeTemporary = (CAMERA_DEVICE)index;
}

void BrillouinAcquisition::on_actionLoad_Voltage_Position_calibration_triggered() {
	m_calibrationFilePath = QFileDialog::getOpenFileName(this, tr("Select Voltage-Position map"),
		QString::fromStdString(m_calibrationFilePath), tr("Calibration map (*.h5)")).toStdString();
	m_scanControl->loadVoltagePositionCalibration(m_calibrationFilePath);
}

void BrillouinAcquisition::initBeampathButtons() {
	for (auto widget : ui->beamPathBox->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) {
		delete widget;
	}

	QLayout* layout = ui->beamPathBox->layout();
	if (layout != 0) {
		QLayoutItem *item;
		while ((item = layout->takeAt(0)) != 0) {
			layout->removeItem(item);
		}
		delete item;
		delete layout;
	}

	QMetaObject::Connection connection;
	QVBoxLayout *verticalLayout = new QVBoxLayout;
	verticalLayout->setAlignment(Qt::AlignTop);
	std::string buttonLabel;
	presetButtons.clear();
	if (m_scanControl->m_availablePresets.size() > 0) {
		QHBoxLayout *presetLayoutLabel = new QHBoxLayout();
		std::string presetLabelString = "Presets:";
		QLabel *presetLabel = new QLabel(presetLabelString.c_str());
		presetLayoutLabel->addWidget(presetLabel);
		verticalLayout->addLayout(presetLayoutLabel);
		QHBoxLayout *layout = new QHBoxLayout();
		layout->setAlignment(Qt::AlignLeft);
		for (gsl::index ii = 0; ii < m_scanControl->m_availablePresets.size(); ii++) {
			QPushButton *button = new QPushButton(m_scanControl->m_presetLabels[m_scanControl->m_availablePresets[ii]].c_str());
			button->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
			button->setMinimumWidth(90);
			button->setMaximumWidth(90);
			layout->addWidget(button);

			connection = QObject::connect(button, &QPushButton::clicked, [=] {
				setPreset((ScanControl::SCAN_PRESET)m_scanControl->m_availablePresets[ii]);
			});
			presetButtons.push_back(button);
		}
		verticalLayout->addLayout(layout);
	}
	elementButtons.clear();
	for (gsl::index ii = 0; ii < m_scanControl->m_deviceElements.deviceCount(); ii++) {
		QHBoxLayout *layout = new QHBoxLayout();

		layout->setAlignment(Qt::AlignLeft);
		QLabel *groupLabel = new QLabel(m_scanControl->m_deviceElements.getDeviceElements()[ii].name.c_str());
		groupLabel->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
		groupLabel->setMinimumWidth(80);
		groupLabel->setMaximumWidth(80);
		layout->addWidget(groupLabel);
		std::vector<QPushButton*> buttons;
		for (gsl::index jj = 0; jj < m_scanControl->m_deviceElements.getDeviceElements()[ii].maxOptions; jj++) {
			buttonLabel = std::to_string(jj + 1);
			QPushButton *button = new QPushButton(buttonLabel.c_str());
			button->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
			button->setMinimumWidth(40);
			button->setMaximumWidth(40);
			layout->addWidget(button);

			connection = QObject::connect(button, &QPushButton::clicked, [=] {
				setElement(m_scanControl->m_deviceElements.getDeviceElements()[ii], jj + 1);
			});
			buttons.push_back(button);
		}
		elementButtons.push_back(buttons);
		verticalLayout->addLayout(layout);
	}
	ui->beamPathBox->setLayout(verticalLayout);
	ui->beamPathBox->show();
}

void BrillouinAcquisition::initScanControl() {
	// deinitialize scanner if necessary
	if (m_scanControl) {
		m_scanControl->deleteLater();
		m_scanControl = nullptr;
	}

	// initialize correct scanner type
	switch (m_scanControllerType) {
		case ScanControl::SCAN_DEVICE::ZEISSECU:
			m_scanControl = new ZeissECU();
			ui->actionLoad_Voltage_Position_calibration->setVisible(false);
			break;
		case ScanControl::SCAN_DEVICE::NIDAQ:
			m_scanControl = new NIDAQ();
			ui->actionLoad_Voltage_Position_calibration->setVisible(true);
			break;
		default:
			m_scanControl = new ZeissECU();
			ui->actionLoad_Voltage_Position_calibration->setVisible(false);
			break;
	}

	// reestablish m_scanControl connections
	QMetaObject::Connection connection = QWidget::connect(
		m_scanControl,
		SIGNAL(connectedDevice(bool)),
		this,
		SLOT(microscopeConnectionChanged(bool))
	);

	// slot to update microscope element button background color
	connection = QWidget::connect(
		m_scanControl,
		SIGNAL(elementPositionsChanged(std::vector<int>)),
		this,
		SLOT(microscopeElementPositionsChanged(std::vector<int>))
	);
	connection = QWidget::connect(
		m_scanControl,
		SIGNAL(elementPositionChanged(DeviceElement, int)),
		this,
		SLOT(microscopeElementPositionChanged(DeviceElement, int))
	);
	connection = QWidget::connect(
		m_scanControl,
		SIGNAL(currentPosition(POINT3)),
		this,
		SLOT(showPosition(POINT3))
	);
	connection = QWidget::connect(
		&buttonDelegate,
		SIGNAL(deletePosition(int)),
		m_scanControl,
		SLOT(deleteSavedPosition(int))
	);
	connection = QWidget::connect(
		&buttonDelegate,
		SIGNAL(moveToPosition(int)),
		m_scanControl,
		SLOT(moveToSavedPosition(int))
	);
	connection = QWidget::connect(
		m_scanControl,
		SIGNAL(savedPositionsChanged(std::vector<POINT3>)),
		tableModel,
		SLOT(setStorage(std::vector<POINT3>))
	);
	connection = QWidget::connect(
		m_scanControl,
		SIGNAL(homePositionBoundsChanged(BOUNDS)),
		this,
		SLOT(setHomePositionBounds(BOUNDS))
	);
	connection = QWidget::connect(
		m_scanControl,
		SIGNAL(currentPositionBoundsChanged(BOUNDS)),
		this,
		SLOT(setCurrentPositionBounds(BOUNDS))
	);
	tableModel->setStorage(m_scanControl->getSavedPositionsNormalized());

	m_acquisitionThread.startWorker(m_scanControl);

	QMetaObject::invokeMethod(m_scanControl, "connectDevice", Qt::AutoConnection);
}

void BrillouinAcquisition::initCamera() {
	// deinitialize camera if necessary
	if (m_pointGrey) {
		m_pointGrey->deleteLater();
		m_pointGrey = nullptr;
	}

	// initialize correct camera type
	QIcon icon(":/BrillouinAcquisition/assets/00disconnected.png");
	switch (m_cameraType) {
		case CAMERA_DEVICE::NONE:
			m_pointGrey = nullptr;
			ui->actionConnect_Brightfield_camera->setVisible(false);
			ui->settingsWidget->removeTab(3);
			break;
		case CAMERA_DEVICE::POINTGREY:
			m_pointGrey = new PointGrey();
			ui->actionConnect_Brightfield_camera->setVisible(true);
			ui->settingsWidget->addTab(ui->ODTcameraTab, "ODT Camera");
			ui->settingsWidget->setTabIcon(3, icon);
			break;
		default:
			m_pointGrey = nullptr;
			ui->actionConnect_Brightfield_camera->setVisible(false);
			ui->settingsWidget->removeTab(3);
			break;
	}

	// don't do anything if no camera is connected
	if (m_cameraType == CAMERA_DEVICE::NONE) {
		return;
	}

	// reestablish camera connections
	QMetaObject::Connection connection = QWidget::connect(
		m_pointGrey,
		SIGNAL(connectedDevice(bool)),
		this,
		SLOT(brightfieldCameraConnectionChanged(bool))
	);

	connection = QWidget::connect(
		m_pointGrey,
		&PointGrey::s_previewBufferSettingsChanged,
		this,
		[this] { updatePlotLimits(m_ODTPlot, m_pointGrey->previewBuffer->m_bufferSettings.roi); }
	);

	connection = QWidget::connect(
		m_pointGrey,
		SIGNAL(s_previewRunning(bool)),
		this,
		SLOT(showBrightfieldPreviewRunning(bool))
	);

	m_acquisitionThread.startWorker(m_pointGrey);

	QMetaObject::invokeMethod(m_pointGrey, "connectDevice", Qt::AutoConnection);
}

void BrillouinAcquisition::microscopeElementPositionsChanged(std::vector<int> positions) {
	m_deviceElementPositions = positions;
	checkElementButtons();
}

void BrillouinAcquisition::microscopeElementPositionChanged(DeviceElement element, int position) {
	m_deviceElementPositions[element.index] = position;
	checkElementButtons();
}

void BrillouinAcquisition::checkElementButtons() {
	for (gsl::index ii = 0; ii < elementButtons.size(); ii++) {
		for (gsl::index jj = 0; jj < elementButtons[ii].size(); jj++) {
			if (m_deviceElementPositions[ii] == jj + 1) {
				elementButtons[ii][jj]->setProperty("class", "active");
			} else {
				elementButtons[ii][jj]->setProperty("class", "");
			}
			elementButtons[ii][jj]->style()->unpolish(elementButtons[ii][jj]);
			elementButtons[ii][jj]->style()->polish(elementButtons[ii][jj]);
			elementButtons[ii][jj]->update();
		}
	}
	for (gsl::index ii = 0; ii < m_scanControl->m_availablePresets.size(); ii++) {
		if (m_scanControl->m_presets[ii] == m_deviceElementPositions) {
			presetButtons[ii]->setProperty("class", "active");
		} else {
			presetButtons[ii]->setProperty("class", "");
		}
		presetButtons[ii]->style()->unpolish(presetButtons[ii]);
		presetButtons[ii]->style()->polish(presetButtons[ii]);
		presetButtons[ii]->update();
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
	if (!m_andor->m_isPreviewRunning) {
		QMetaObject::invokeMethod(m_andor, "startPreview", Qt::QueuedConnection, Q_ARG(CAMERA_SETTINGS, m_acquisitionSettings.camera));
	} else {
		m_andor->m_isPreviewRunning = false;
	}
}

void BrillouinAcquisition::on_camera_singleShot_clicked() {
	QMetaObject::invokeMethod(m_andor, "acquireSingle", Qt::QueuedConnection);
}

void BrillouinAcquisition::on_acquisitionStart_clicked() {
	// set camera ROI
	m_acquisitionSettings.camera.roi = m_deviceSettings.camera.roi;
	if (!m_acquisition->isAcqRunning()) {
		QMetaObject::invokeMethod(m_acquisition, "startAcquisition", Qt::QueuedConnection, Q_ARG(ACQUISITION_SETTINGS, m_acquisitionSettings));
	} else {
		m_acquisition->m_abort = 1;
	}
}

void BrillouinAcquisition::updateFilename(std::string filename) {
	m_acquisitionSettings.filename = filename;
	updateAcquisitionSettings();
}

void BrillouinAcquisition::updateAcquisitionSettings() {
	ui->acquisitionFilename->setText(QString::fromStdString(m_acquisitionSettings.filename));

	// AOI settings
	ui->startX->setValue(m_acquisitionSettings.xMin);
	ui->startY->setValue(m_acquisitionSettings.yMin);
	ui->startZ->setValue(m_acquisitionSettings.zMin);
	ui->endX->setValue(m_acquisitionSettings.xMax);
	ui->endY->setValue(m_acquisitionSettings.yMax);
	ui->endZ->setValue(m_acquisitionSettings.zMax);
	ui->stepsX->setValue(m_acquisitionSettings.xSteps);
	ui->stepsY->setValue(m_acquisitionSettings.ySteps);
	ui->stepsZ->setValue(m_acquisitionSettings.zSteps);

	// calibration settings
	ui->preCalibration->setChecked(m_acquisitionSettings.preCalibration);
	ui->postCalibration->setChecked(m_acquisitionSettings.postCalibration);
	ui->conCalibration->setChecked(m_acquisitionSettings.conCalibration);
	ui->conCalibrationInterval->setValue(m_acquisitionSettings.conCalibrationInterval);
	ui->nrCalibrationImages->setValue(m_acquisitionSettings.nrCalibrationImages);
	ui->calibrationExposureTime->setValue(m_acquisitionSettings.calibrationExposureTime);
	ui->sampleSelection->setCurrentText(QString::fromStdString(m_acquisitionSettings.sample));

}

void BrillouinAcquisition::on_startX_valueChanged(double value) {
	m_acquisitionSettings.xMin = value;
}

void BrillouinAcquisition::on_startY_valueChanged(double value) {
	m_acquisitionSettings.yMin = value;
}

void BrillouinAcquisition::on_startZ_valueChanged(double value) {
	m_acquisitionSettings.zMin = value;
}

void BrillouinAcquisition::on_endX_valueChanged(double value) {
	m_acquisitionSettings.xMax = value;
}

void BrillouinAcquisition::on_endY_valueChanged(double value) {
	m_acquisitionSettings.yMax = value;
}

void BrillouinAcquisition::on_endZ_valueChanged(double value) {
	m_acquisitionSettings.zMax = value;
}

void BrillouinAcquisition::on_stepsX_valueChanged(int value) {
	m_acquisitionSettings.xSteps = value;
}

void BrillouinAcquisition::on_stepsY_valueChanged(int value) {
	m_acquisitionSettings.ySteps = value;
}

void BrillouinAcquisition::on_stepsZ_valueChanged(int value) {
	m_acquisitionSettings.zSteps = value;
}

void BrillouinAcquisition::on_preCalibration_stateChanged(int state) {
	m_acquisitionSettings.preCalibration = (bool)state;
}

void BrillouinAcquisition::on_postCalibration_stateChanged(int state) {
	m_acquisitionSettings.postCalibration = (bool)state;
}

void BrillouinAcquisition::on_conCalibration_stateChanged(int state) {
	m_acquisitionSettings.conCalibration = (bool)state;
}


void BrillouinAcquisition::on_sampleSelection_currentIndexChanged(const QString &text) {
	m_acquisitionSettings.sample = text.toStdString();
};

void BrillouinAcquisition::on_conCalibrationInterval_valueChanged(double value) {
	m_acquisitionSettings.conCalibrationInterval = value;
}

void BrillouinAcquisition::on_nrCalibrationImages_valueChanged(int value) {
	m_acquisitionSettings.nrCalibrationImages = value;
};

void BrillouinAcquisition::on_calibrationExposureTime_valueChanged(double value) {
	m_acquisitionSettings.calibrationExposureTime = value;
};

/*
 * Functions regarding the repetition feature.
 */

void BrillouinAcquisition::on_repetitionCount_valueChanged(int count) {
	m_acquisitionSettings.repetitions.count = count;
};

void BrillouinAcquisition::on_repetitionInterval_valueChanged(double interval) {
	m_acquisitionSettings.repetitions.interval = interval;
};

void BrillouinAcquisition::showRepProgress(int repNumber, int timeToNext) {
	ui->repetitionProgress->setValue(100 * ((double)repNumber + 1) / m_acquisitionSettings.repetitions.count);

	QString string;
	if (timeToNext > 0) {
		string = formatSeconds(timeToNext) + " to next repetition.";
	} else {
		if (repNumber < m_acquisitionSettings.repetitions.count) {
			string.sprintf("Measuring repetition %1.0d of %1.0d.", repNumber + 1, m_acquisitionSettings.repetitions.count);
		} else {
			string.sprintf("Finished %1.0d repetitions.", m_acquisitionSettings.repetitions.count);
		}
	}
	ui->repetitionProgress->setFormat(string);
};

void BrillouinAcquisition::on_savePosition_clicked() {
	QMetaObject::invokeMethod(m_scanControl, "savePosition", Qt::AutoConnection);
}

void BrillouinAcquisition::on_setHome_clicked() {
	QMetaObject::invokeMethod(m_scanControl, "setHome", Qt::AutoConnection);
}

void BrillouinAcquisition::on_moveHome_clicked() {
	if (!m_measurementRunning) {
		QMetaObject::invokeMethod(m_scanControl, "moveHome", Qt::AutoConnection);
	}
}

void BrillouinAcquisition::on_setPositionX_valueChanged(double positionX) {
	if (!m_measurementRunning) {
		QMetaObject::invokeMethod(m_scanControl, "setPositionRelativeX", Qt::AutoConnection, Q_ARG(double, positionX));
	}
}

void BrillouinAcquisition::on_setPositionY_valueChanged(double positionY) {
	if (!m_measurementRunning) {
		QMetaObject::invokeMethod(m_scanControl, "setPositionRelativeY", Qt::AutoConnection, Q_ARG(double, positionY));
	}
}

void BrillouinAcquisition::on_setPositionZ_valueChanged(double positionZ) {
	if (!m_measurementRunning) {
		QMetaObject::invokeMethod(m_scanControl, "setPositionRelativeZ", Qt::AutoConnection, Q_ARG(double, positionZ));
	}
}

void BrillouinAcquisition::updateSavedPositions() {
	ui->tableView->setModel(tableModel);
	ui->tableView->setItemDelegateForColumn(3, &buttonDelegate);
	ui->tableView->verticalHeader()->setDefaultSectionSize(35);
	ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui->tableView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	ui->tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	ui->tableView->show();
}

void BrillouinAcquisition::on_exposureTime_valueChanged(double value) {
	m_acquisitionSettings.camera.exposureTime = value;
};

void BrillouinAcquisition::on_frameCount_valueChanged(int value) {
	m_acquisitionSettings.camera.frameCount = value;
}
void BrillouinAcquisition::on_selectFolder_clicked() {
	m_acquisitionSettings.folder = QFileDialog::getExistingDirectory(this, tr("Select directory to store measurements"),
		QString::fromStdString(m_acquisitionSettings.folder), QFileDialog::ShowDirsOnly	| QFileDialog::DontResolveSymlinks).toStdString();
};

void BrillouinAcquisition::setColormap(QCPColorGradient *gradient, CustomGradientPreset preset) {
	gradient->clearColorStops();
	switch (preset) {
		case gpParula:
			gradient->setColorInterpolation(QCPColorGradient::ciRGB);
			gradient->setColorStopAt(0.00, QColor( 53,  42, 135));
			gradient->setColorStopAt(0.05, QColor( 53,  62, 175));
			gradient->setColorStopAt(0.10, QColor( 27,  85, 215));
			gradient->setColorStopAt(0.15, QColor(  2, 106, 225));
			gradient->setColorStopAt(0.20, QColor( 15, 119, 219));
			gradient->setColorStopAt(0.25, QColor( 20, 132, 212));
			gradient->setColorStopAt(0.30, QColor( 13, 147, 210));
			gradient->setColorStopAt(0.35, QColor(  6, 160, 205));
			gradient->setColorStopAt(0.40, QColor(  7, 170, 193));
			gradient->setColorStopAt(0.45, QColor( 24, 177, 178));
			gradient->setColorStopAt(0.50, QColor( 51, 184, 161));
			gradient->setColorStopAt(0.55, QColor( 85, 189, 142));
			gradient->setColorStopAt(0.60, QColor(122, 191, 124));
			gradient->setColorStopAt(0.65, QColor(155, 191, 111));
			gradient->setColorStopAt(0.70, QColor(184, 189,  99));
			gradient->setColorStopAt(0.75, QColor(211, 187,  88));
			gradient->setColorStopAt(0.80, QColor(236, 185,  76));
			gradient->setColorStopAt(0.85, QColor(255, 193,  58));
			gradient->setColorStopAt(0.90, QColor(250, 209,  43));
			gradient->setColorStopAt(0.95, QColor(245, 227,  30));
			gradient->setColorStopAt(1.00, QColor(249, 251,  14));
			break;
		case gpGrayscale:
			gradient->loadPreset(QCPColorGradient::gpGrayscale);
			break;
		default:
			gradient->loadPreset(QCPColorGradient::gpGrayscale);
			break;
	}
}
