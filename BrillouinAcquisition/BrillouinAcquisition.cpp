#include "stdafx.h"
#include "BrillouinAcquisition.h"
#include "version.h"
#include "logger.h"
#include "simplemath.h"

BrillouinAcquisition::BrillouinAcquisition(QWidget *parent):
	QMainWindow(parent), ui(new Ui::BrillouinAcquisitionClass) {
	ui->setupUi(this);

	QWidget::connect(
		m_andor,
		SIGNAL(acquisitionRunning(bool, CircularBuffer<AT_U8>*, AT_64, AT_64)),
		this,
		SLOT(acquisitionRunning(bool, CircularBuffer<AT_U8>*, AT_64, AT_64))
	);

	QWidget::connect(
		m_andor,
		SIGNAL(optionsChanged(CAMERA_OPTIONS)),
		this,
		SLOT(cameraOptionsChanged(CAMERA_OPTIONS))
	);

	QWidget::connect(
		m_andor,
		SIGNAL(settingsChanged(CAMERA_SETTINGS)),
		this,
		SLOT(cameraSettingsChanged(CAMERA_SETTINGS))
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

	// slots to update the settings inputs
	QWidget::connect(
		this,
		SIGNAL(settingsCameraChanged(SETTINGS_DEVICES)),
		this,
		SLOT(settingsCameraUpdate(SETTINGS_DEVICES))
	);

	// slot microscope connection
	QWidget::connect(
		m_scanControl,
		SIGNAL(microscopeConnected(bool)),
		this,
		SLOT(microscopeConnectionChanged(bool))
	);

	// slot to update microscope element button background color
	QWidget::connect(
		m_scanControl->m_stand,
		SIGNAL(elementPositionsChanged(std::vector<int>)),
		this,
		SLOT(microscopeElementPositionsChanged(std::vector<int>))
	);
	QWidget::connect(
		m_scanControl->m_stand,
		SIGNAL(elementPositionsChanged(int, int)),
		this,
		SLOT(microscopeElementPositionsChanged(int, int))
	);

	qRegisterMetaType<std::string>("std::string");
	qRegisterMetaType<AT_64>("AT_64");
	qRegisterMetaType<CircularBuffer<AT_U8>>("CircularBuffer<AT_U8>");

	QIcon icon(":/BrillouinAcquisition/assets/00disconnected.png");
	ui->settingsWidget->setTabIcon(0, icon);
	ui->settingsWidget->setTabIcon(1, icon);
	ui->settingsWidget->setTabIcon(2, icon);
	ui->settingsWidget->setIconSize(QSize(16, 16));

	ui->actionEnable_Cooling->setEnabled(FALSE);

	// start camera worker thread
	m_cameraThread.startWorker(m_andor);
	m_microscopeThread.startWorker(m_scanControl);
	m_scanControl->m_comObject->moveToThread(&m_microscopeThread);

	// set up the camera image plot
	BrillouinAcquisition::initializePlot();

	writeExampleH5bmFile();

	// Set up GUI
	std::vector<std::string> groupLabels = {"Reflector", "Objective", "Tubelens", "Baseport", "Sideport", "Mirror"};
	std::vector<int> maxOptions = { 5, 6, 3, 3, 3, 2 };
	QVBoxLayout *verticalLayout = new QVBoxLayout;
	verticalLayout->setAlignment(Qt::AlignTop);
	std::string buttonLabel;
	QHBoxLayout *presetLayoutLabel = new QHBoxLayout();
	std::string presetLabelString = "Presets:";
	QLabel *presetLabel = new QLabel(presetLabelString.c_str());
	presetLayoutLabel->addWidget(presetLabel);
	verticalLayout->addLayout(presetLayoutLabel);
	QHBoxLayout *layout = new QHBoxLayout();
	for (int ii = 0; ii < presetLabels.size(); ii++) {
		buttonLabel = std::to_string(ii + 1);
		QPushButton *button = new QPushButton(presetLabels[ii].c_str());
		button->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
		button->setMinimumWidth(90);
		button->setMaximumWidth(90);
		layout->addWidget(button);

		QObject::connect(button, &QPushButton::clicked, [=] {
			setPreset(ii);
		});
		presetButtons.push_back(button);
	}
	verticalLayout->addLayout(layout);
	for (int ii = 0; ii < groupLabels.size(); ii++) {
		QHBoxLayout *layout = new QHBoxLayout();

		layout->setAlignment(Qt::AlignLeft);
		QLabel *groupLabel = new QLabel(groupLabels[ii].c_str());
		groupLabel->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
		groupLabel->setMinimumWidth(80);
		groupLabel->setMaximumWidth(80);
		layout->addWidget(groupLabel);
		std::vector<QPushButton*> buttons;
		for (int jj = 0; jj < maxOptions[ii]; jj++) {
			buttonLabel = std::to_string(jj + 1);
			QPushButton *button = new QPushButton(buttonLabel.c_str());
			button->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
			button->setMinimumWidth(40);
			button->setMaximumWidth(40);
			layout->addWidget(button);

			QObject::connect(button, &QPushButton::clicked, [=] {
				setElement(ii, jj+1);
			});
			buttons.push_back(button);
		}
		elementButtons.push_back(buttons);
		verticalLayout->addLayout(layout);
	}
	ui->beamPathBox->setLayout(verticalLayout);

	ui->parametersWidget->layout()->setAlignment(Qt::AlignTop);
}

BrillouinAcquisition::~BrillouinAcquisition() {
	m_cameraThread.exit();
	m_cameraThread.wait();
	m_microscopeThread.exit();
	m_microscopeThread.wait();
	m_storageThread.exit();
	m_storageThread.wait();
	delete m_andor;
	qInfo(logInfo()) << "BrillouinAcquisition closed.";
	delete ui;
}

void BrillouinAcquisition::setElement(int element, int position) {
	switch (element) {
		case 0:
			QMetaObject::invokeMethod(m_scanControl->m_stand, "setReflector", Qt::QueuedConnection, Q_ARG(int, position));
			break;
		case 1:
			QMetaObject::invokeMethod(m_scanControl->m_stand, "setObjective", Qt::QueuedConnection, Q_ARG(int, position));
			break;
		case 2:
			QMetaObject::invokeMethod(m_scanControl->m_stand, "setTubelens", Qt::QueuedConnection, Q_ARG(int, position));
			break;
		case 3:
			QMetaObject::invokeMethod(m_scanControl->m_stand, "setBaseport", Qt::QueuedConnection, Q_ARG(int, position));
			break;
		case 4:
			QMetaObject::invokeMethod(m_scanControl->m_stand, "setSideport", Qt::QueuedConnection, Q_ARG(int, position));
			break;
		case 5:
			QMetaObject::invokeMethod(m_scanControl->m_stand, "setMirror", Qt::QueuedConnection, Q_ARG(int, position));
			break;
	}
}

void BrillouinAcquisition::setPreset(int preset) {
	QMetaObject::invokeMethod(m_scanControl->m_stand, "setReflector", Qt::QueuedConnection, Q_ARG(int, microscope_presets[preset][0]));
	QMetaObject::invokeMethod(m_scanControl->m_stand, "setObjective", Qt::QueuedConnection, Q_ARG(int, microscope_presets[preset][1]));
	QMetaObject::invokeMethod(m_scanControl->m_stand, "setTubelens", Qt::QueuedConnection, Q_ARG(int, microscope_presets[preset][2]));
	QMetaObject::invokeMethod(m_scanControl->m_stand, "setBaseport", Qt::QueuedConnection, Q_ARG(int, microscope_presets[preset][3]));
	QMetaObject::invokeMethod(m_scanControl->m_stand, "setSideport", Qt::QueuedConnection, Q_ARG(int, microscope_presets[preset][4]));
	QMetaObject::invokeMethod(m_scanControl->m_stand, "setMirror", Qt::QueuedConnection, Q_ARG(int, microscope_presets[preset][5]));
}

void BrillouinAcquisition::acquisitionRunning(bool isRunning, CircularBuffer<AT_U8>* liveBuffer, AT_64 imageWidth, AT_64 imageHeight) {
	if (m_liveBuffer) {
		delete m_liveBuffer;
	}
	m_liveBuffer = liveBuffer;
	if (isRunning) {
		ui->camera_playPause->setText("Stop");
		m_viewRunning = TRUE;
		m_imageWidth = imageWidth;
		m_imageHeight = imageHeight;
		onNewImage();
	} else {
		ui->camera_playPause->setText("Play");
		m_viewRunning = FALSE;
	}
}

void BrillouinAcquisition::cameraOptionsChanged(CAMERA_OPTIONS options) {
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
	ui->ROITop->setMinimum(options.ROIHeightLimits[0]);
	ui->ROITop->setMaximum(options.ROIHeightLimits[1]);
	ui->ROIWidth->setMinimum(options.ROIWidthLimits[0]);
	ui->ROIWidth->setMaximum(options.ROIWidthLimits[1]);
	ui->ROILeft->setMinimum(options.ROIWidthLimits[0]);
	ui->ROILeft->setMaximum(options.ROIWidthLimits[1]);
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
	ui->ROILeft->setValue(settings.roi.left);
	ui->ROIWidth->setValue(settings.roi.width);
	ui->ROITop->setValue(settings.roi.top);
	ui->ROIHeight->setValue(settings.roi.height);

	ui->triggerMode->setCurrentText(QString::fromWCharArray(settings.triggerMode));
	ui->binning->setCurrentText(QString::fromWCharArray(settings.roi.binning));

	ui->pixelReadoutRate->setCurrentText(QString::fromWCharArray(settings.readout.pixelReadoutRate));
	ui->cycleMode->setCurrentText(QString::fromWCharArray(settings.readout.cycleMode));
	ui->preAmpGain->setCurrentText(QString::fromWCharArray(settings.readout.preAmpGain));
	ui->pixelEncoding->setCurrentText(QString::fromWCharArray(settings.readout.pixelEncoding));
}

void BrillouinAcquisition::writeExampleH5bmFile() {
	m_h5bm = new H5BM(0, "Brillouin-0.h5", H5F_ACC_RDWR);

	std::string now = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
		.toString(Qt::ISODate).toStdString();

	m_h5bm->setDate(now);
	std::string date = m_h5bm->getDate();

	std::string version = m_h5bm->getVersion();

	std::string commentIn = "Brillouin data";
	m_h5bm->setComment(commentIn);
	std::string commentOut = m_h5bm->getComment();

	int resolutionX, resolutionY, resolutionZ;

	resolutionX = 11;
	resolutionY = 13;
	resolutionZ = 2;

	m_h5bm->setResolution("x", resolutionX);
	m_h5bm->setResolution("y", resolutionY);
	m_h5bm->setResolution("z", resolutionZ);

	int resolutionXout = m_h5bm->getResolution("x");

	// Create position vector
	double minX, maxX, minY, maxY, minZ, maxZ;
	minX = -10;
	maxX = 10;
	minY = -20;
	maxY = 20;
	minZ = 0;
	maxZ = 10;
	int nrPositions = resolutionX * resolutionY * resolutionZ;
	std::vector<double> positionsX(nrPositions);
	std::vector<double> positionsY(nrPositions);
	std::vector<double> positionsZ(nrPositions);
	std::vector<double> posX = simplemath::linspace(minX, maxX, resolutionX);
	std::vector<double> posY = simplemath::linspace(minY, maxY, resolutionY);
	std::vector<double> posZ = simplemath::linspace(minZ, maxZ, resolutionZ);
	int ll = 0;
	for (int ii = 0; ii < resolutionZ; ii++) {
		for (int jj = 0; jj < resolutionX; jj++) {
			for (int kk = 0; kk < resolutionY; kk++) {
				positionsX[ll] = posX[jj];
				positionsY[ll] = posY[kk];
				positionsZ[ll] = posZ[ii];
				ll++;
			}
		}
	}

	int rank = 3;
	// For compatibility with MATLAB respect Fortran-style ordering: z, x, y
	hsize_t *dims = new hsize_t[rank];
	dims[0] = resolutionZ;
	dims[1] = resolutionX;
	dims[2] = resolutionY;

	m_h5bm->setPositions("x", positionsX, rank, dims);
	m_h5bm->setPositions("y", positionsY, rank, dims);
	m_h5bm->setPositions("z", positionsZ, rank, dims);
	delete[] dims;

	positionsX = m_h5bm->getPositions("x");
	positionsY = m_h5bm->getPositions("y");
	positionsZ = m_h5bm->getPositions("z");

	// set payload data
	std::vector<double> data(1600);
	int rank_data = 3;
	hsize_t nrImages = 2;
	hsize_t imageWidth = 100;
	hsize_t imageHeight = 80;
	hsize_t dims_data[3] = {nrImages, imageHeight, imageWidth};
	for (int ii = 0; ii < resolutionZ; ii++) {
		for (int jj = 0; jj < resolutionX; jj++) {
			for (int kk = 0; kk < resolutionY; kk++) {
				m_h5bm->setPayloadData(jj, kk, ii, data, rank, dims_data);
			}
		}
	}

	data = m_h5bm->getPayloadData(1, 1, 1);
	date = m_h5bm->getPayloadDate(1, 1, 1);

	m_h5bm->setBackgroundData(data, rank, dims_data, "2016-05-06T11:11:00+02:00");
	data = m_h5bm->getBackgroundData();
	date = m_h5bm->getBackgroundDate();

	m_h5bm->setCalibrationData(1, data, rank, dims_data, "methanol", 3.799);
	m_h5bm->setCalibrationData(2, data, rank, dims_data, "water", 5.088);

	delete m_h5bm;
}

void BrillouinAcquisition::initializePlot() {
	// configure axis rect
	QCustomPlot *customPlot = ui->customplot;

	customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom); // this will also allow rescaling the color scale by dragging/zooming
	customPlot->axisRect()->setupFullAxesBox(true);
	//customPlot->xAxis->setLabel("x");
	//customPlot->yAxis->setLabel("y");

	// this are the selection modes
	customPlot->setSelectionRectMode(QCP::srmZoom);	// allows to select region by rectangle
	customPlot->setSelectionRectMode(QCP::srmNone);	// allows to drag the position

	// set background color to default light gray
	customPlot->setBackground(QColor(240, 240, 240, 255));
	customPlot->axisRect()->setBackground(Qt::white);

	// set up the QCPColorMap:
	m_colorMap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);
	m_colorMap->data()->setSize(m_imageWidth, m_imageHeight); // number of data points
	m_colorMap->data()->setRange(QCPRange(0, m_imageWidth), QCPRange(0, m_imageHeight));

	// fill map with zero
	m_colorMap->data()->fill(0);

	// turn off interpolation
	m_colorMap->setInterpolate(false);

	// add a color scale:
	QCPColorScale *colorScale = new QCPColorScale(customPlot);
	customPlot->plotLayout()->addElement(0, 1, colorScale); // add it to the right of the main axis rect
	colorScale->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
	m_colorMap->setColorScale(colorScale); // associate the color map with the color scale
	colorScale->axis()->setLabel("Intensity");

	// set the color gradient of the color map to one of the presets:
	QCPColorGradient gradient = QCPColorGradient();
	setColormap(&gradient, gpParula);
	m_colorMap->setGradient(gradient);

	// rescale the data dimension (color) such that all data points lie in the span visualized by the color gradient:
	m_colorMap->rescaleDataRange();

	// make sure the axis rect and color scale synchronize their bottom and top margins (so they line up):
	QCPMarginGroup *marginGroup = new QCPMarginGroup(customPlot);
	customPlot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
	colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);

	// rescale the key (x) and value (y) axes so the whole color map is visible:
	customPlot->rescaleAxes();
}

void BrillouinAcquisition::xAxisRangeChanged(const QCPRange &newRange) {
	// checks for certain range
	ui->customplot->xAxis->setRange(newRange.bounded(0, 2048));
	if (newRange.lower >= 0) {
		m_deviceSettings.camera.roi.left = newRange.lower;
	}
	int width = newRange.upper - newRange.lower;
	if (width < 2048) {
		m_deviceSettings.camera.roi.width = width;
	}
	emit(settingsCameraChanged(m_deviceSettings));
}

void BrillouinAcquisition::yAxisRangeChanged(const QCPRange &newRange) {
	// checks for certain range
	ui->customplot->yAxis->setRange(newRange.bounded(0, 2048));
	if (newRange.lower >= 0) {
		m_deviceSettings.camera.roi.top = newRange.lower;
	}
	int height = newRange.upper - newRange.lower;
	if (height < 2048) {
		m_deviceSettings.camera.roi.height = height;
	}
	emit(settingsCameraChanged(m_deviceSettings));
}

void BrillouinAcquisition::settingsCameraUpdate(SETTINGS_DEVICES settings) {
	ui->ROILeft->setValue(settings.camera.roi.left);
	ui->ROIWidth->setValue(settings.camera.roi.width);
	ui->ROITop->setValue(settings.camera.roi.top);
	ui->ROIHeight->setValue(settings.camera.roi.height);
}

void BrillouinAcquisition::on_ROILeft_valueChanged(int left) {
	m_deviceSettings.camera.roi.left = left;
	ui->customplot->xAxis->setRange(QCPRange(left, left + m_deviceSettings.camera.roi.width));
	ui->customplot->replot();
}

void BrillouinAcquisition::on_ROIWidth_valueChanged(int width) {
	m_deviceSettings.camera.roi.width = width;
	ui->customplot->xAxis->setRange(QCPRange(m_deviceSettings.camera.roi.left, m_deviceSettings.camera.roi.left + width));
	ui->customplot->replot();
}

void BrillouinAcquisition::on_ROITop_valueChanged(int top) {
	m_deviceSettings.camera.roi.top = top;
	ui->customplot->yAxis->setRange(QCPRange(top, top + m_deviceSettings.camera.roi.height));
	ui->customplot->replot();
}

void BrillouinAcquisition::on_ROIHeight_valueChanged(int height) {
	m_deviceSettings.camera.roi.height = height;
	ui->customplot->yAxis->setRange(QCPRange(m_deviceSettings.camera.roi.top, m_deviceSettings.camera.roi.top + height));
	ui->customplot->replot();
}

void BrillouinAcquisition::onNewImage() {
	if (m_liveBuffer && m_viewRunning) {
		// if no image is ready return immediately
		if (!m_liveBuffer->m_usedBuffers->tryAcquire()) {
			QMetaObject::invokeMethod(this, "onNewImage", Qt::QueuedConnection);
			return;
		}

		unsigned short* unpackedBuffer = reinterpret_cast<unsigned short*>(m_liveBuffer->getReadBuffer());

		m_colorMap->data()->setSize(m_imageWidth, m_imageHeight); // we want the color map to have nx * ny data points
		m_colorMap->data()->setRange(QCPRange(0, m_imageWidth), QCPRange(0, m_imageHeight)); // and span the coordinate range -4..4 in both key (x) and value (y) dimensions

		int tIndex;
		for (int xIndex = 0; xIndex < m_imageWidth; ++xIndex) {
			for (int yIndex = 0; yIndex < m_imageHeight; ++yIndex) {
				tIndex = xIndex * m_imageHeight + yIndex;
				m_colorMap->data()->setCell(xIndex, yIndex, unpackedBuffer[tIndex]);
			}
		}
		m_liveBuffer->m_freeBuffers->release();
		m_colorMap->rescaleDataRange();
		ui->customplot->rescaleAxes();
		ui->customplot->replot();
			
		QMetaObject::invokeMethod(this, "onNewImage", Qt::QueuedConnection);
	}
}

void BrillouinAcquisition::on_actionConnect_Camera_triggered() {
	if (m_andor->getConnectionStatus()) {
		m_andor->disconnect();
		ui->actionConnect_Camera->setText("Connect Camera");
		ui->actionEnable_Cooling->setText("Enable Cooling");
		QIcon icon(":/BrillouinAcquisition/assets/00disconnected.png");
		ui->settingsWidget->setTabIcon(0, icon);
		ui->actionEnable_Cooling->setEnabled(FALSE);
		ui->camera_playPause->setEnabled(FALSE);
		ui->camera_singleShot->setEnabled(FALSE);
	} else {
		m_andor->connect();
		ui->actionConnect_Camera->setText("Disconnect Camera");
		QIcon icon(":/BrillouinAcquisition/assets/01standby.png");
		ui->settingsWidget->setTabIcon(0, icon);
		ui->actionEnable_Cooling->setEnabled(TRUE);
		ui->camera_playPause->setEnabled(TRUE);
		ui->camera_singleShot->setEnabled(TRUE);
	}
}

void BrillouinAcquisition::on_actionEnable_Cooling_triggered() {
	if (m_andor->getConnectionStatus()) {
		if (m_andor->getSensorCooling()) {
			m_andor->setSensorCooling(FALSE);
			ui->actionEnable_Cooling->setText("Enable Cooling");
			QIcon icon(":/BrillouinAcquisition/assets/01standby.png");
			ui->settingsWidget->setTabIcon(0, icon);
		} else {
			m_andor->setSensorCooling(TRUE);
			ui->actionEnable_Cooling->setText("Disable Cooling");
			QIcon icon(":/BrillouinAcquisition/assets/02cooling.png");
			ui->settingsWidget->setTabIcon(0, icon);
		}
	}
}

void BrillouinAcquisition::on_actionConnect_Stage_triggered() {
	if (m_scanControl->getConnectionStatus()) {
		QMetaObject::invokeMethod(m_scanControl, "disconnect", Qt::QueuedConnection);
	} else {
		QMetaObject::invokeMethod(m_scanControl, "connect", Qt::QueuedConnection);
	}
}

void BrillouinAcquisition::microscopeConnectionChanged(bool isConnected) {
	if (isConnected) {
		ui->actionConnect_Stage->setText("Disconnect Microscope");
		QIcon icon(":/BrillouinAcquisition/assets/03ready.png");
		ui->settingsWidget->setTabIcon(1, icon);
		ui->settingsWidget->setTabIcon(2, icon);
		QMetaObject::invokeMethod(m_scanControl->m_stand, "getElementPositions", Qt::QueuedConnection);
	} else {
		ui->actionConnect_Stage->setText("Connect Microscope");
		QIcon icon(":/BrillouinAcquisition/assets/00disconnected.png");
		ui->settingsWidget->setTabIcon(1, icon);
		ui->settingsWidget->setTabIcon(2, icon);
		microscopeElementPositionsChanged({ 0,0,0,0,0,0 });
	}
}

void BrillouinAcquisition::microscopeElementPositionsChanged(std::vector<int> positions) {
	microscopeElementPositions = positions;
	checkElementButtons();
}

void BrillouinAcquisition::microscopeElementPositionsChanged(int element, int position) {
	microscopeElementPositions[element] = position;
	checkElementButtons();
}

void BrillouinAcquisition::checkElementButtons() {
	for (int ii = 0; ii < elementButtons.size(); ii++) {
		for (int jj = 0; jj < elementButtons[ii].size(); jj++) {
			if (microscopeElementPositions[ii] == jj + 1) {
				elementButtons[ii][jj]->setProperty("class", "active");
			} else {
				elementButtons[ii][jj]->setProperty("class", "");
			}
			elementButtons[ii][jj]->style()->unpolish(elementButtons[ii][jj]);
			elementButtons[ii][jj]->style()->polish(elementButtons[ii][jj]);
			elementButtons[ii][jj]->update();
		}
	}
	for (int ii = 0; ii < microscope_presets.size(); ii++) {
		if (microscope_presets[ii] == microscopeElementPositions) {
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
	if (!m_andor->m_isAcquiring) {
		QMetaObject::invokeMethod(m_andor, "acquireContinuously", Qt::QueuedConnection);
	} else {
		m_andor->m_isAcquiring = FALSE;
	}
}

void BrillouinAcquisition::on_camera_singleShot_clicked() {
	QMetaObject::invokeMethod(m_andor, "acquireSingle", Qt::QueuedConnection);
}

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
	}
}
