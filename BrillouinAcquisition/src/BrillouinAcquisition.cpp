#include "stdafx.h"
#include "filesystem"

#include "BrillouinAcquisition.h"
#include "version.h"
#include "helper/logger.h"
#include "lib/math/simplemath.h"
#include "lib/colormaps.h"

using namespace std::filesystem;

BrillouinAcquisition::BrillouinAcquisition(QWidget *parent) noexcept :
	QMainWindow(parent), ui(new Ui::BrillouinAcquisitionClass) {
	ui->setupUi(this);

	// Set window title
	auto title = QString{ "BrillouinAcquisition v%1.%2.%3" }.arg(Version::MAJOR).arg(Version::MINOR).arg(Version::PATCH);
	if (Version::PRERELEASE.length() > 0) {
		title += "-" + QString::fromStdString(Version::PRERELEASE);
	}
	#ifdef _DEBUG
		title += QString{ " - Debug" };
	#endif
	this->setWindowTitle(title);

	static QMetaObject::Connection connection;
	// slot to limit the axis of the camera display after user interaction
	connection = QWidget::connect<void(QCPAxis::*)(const QCPRange &)>(
		ui->customplot->xAxis,
		&QCPAxis::rangeChanged,
		this,
		[this](QCPRange newRange) { xAxisRangeChanged(newRange); }
	);
	connection = QWidget::connect<void(QCPAxis::*)(const QCPRange &)>(
		ui->customplot->yAxis,
		&QCPAxis::rangeChanged,
		this,
		[this](QCPRange newRange) { yAxisRangeChanged(newRange); }
	);

	// slot to limit the axis of the camera display after user interaction
	connection = QWidget::connect<void(QCPAxis::*)(const QCPRange &)>(
		ui->customplot_brightfield->xAxis,
		&QCPAxis::rangeChanged,
		this,
		[this](QCPRange newRange) { xAxisRangeChangedODT(newRange); }
	);
	connection = QWidget::connect<void(QCPAxis::*)(const QCPRange &)>(
		ui->customplot_brightfield->yAxis,
		&QCPAxis::rangeChanged,
		this,
		[this](QCPRange newRange) { yAxisRangeChangedODT(newRange); }
	);

	// slot to update filename
	connection = QWidget::connect(
		m_acquisition,
		&Acquisition::s_filenameChanged,
		this,
		[this](std::string filename) { updateFilename(filename); }
	);

	// slot to show current acquisition progress
	connection = QWidget::connect(
		m_acquisition,
		&Acquisition::s_enabledModes,
		this,
		[this](ACQUISITION_MODE modes) { showEnabledModes(modes); }
	);

	// slot to show current acquisition position
	connection = QWidget::connect(
		m_Brillouin,
		&Brillouin::s_positionChanged,
		this,
		[this](POINT3 position, int imageNr) { showAcqPosition(position, imageNr); }
	);

	// slot to show current acquisition state
	connection = QWidget::connect(
		m_Brillouin,
		&Brillouin::s_acquisitionStatus,
		this,
		[this](ACQUISITION_STATUS state) { showBrillouinStatus(state); }
	);

	// slot to show current repetition progress
	connection = QWidget::connect(
		m_Brillouin,
		&Brillouin::s_repetitionProgress,
		this,
		[this](double progress, int seconds) { showBrillouinProgress(progress, seconds); }
	);

	// slot to show calibration running
	connection = QWidget::connect(
		m_Brillouin,
		&Brillouin::s_calibrationRunning,
		this,
		[this](bool isCalibrating) { showCalibrationRunning(isCalibrating); }
	);

	// slot to show time until next calibration
	connection = QWidget::connect(
		m_Brillouin,
		&Brillouin::s_timeToCalibration,
		this,
		[this](int value) { showCalibrationInterval(value); }
	);

	// slot to show repetitions
	connection = QWidget::connect(
		m_Brillouin,
		&Brillouin::s_totalProgress,
		this,
		[this](int repNumber, int timeToNext) { showRepProgress(repNumber, timeToNext); }
	);

	// slot to update the scan order
	connection = QWidget::connect(
		m_Brillouin,
		&Brillouin::s_scanOrderChanged,
		this,
		[this](SCAN_ORDER scanOrder) { scanOrderChanged(scanOrder); }
	);

	// slot to positions in brightfield
	connection = QWidget::connect(
		m_Brillouin,
		&Brillouin::s_orderedPositionsChanged,
		this,
		[this](std::vector<POINT3> orderedPositions) { AOI_changed(orderedPositions); }
	);

	m_Brillouin->determineScanOrder();

	qRegisterMetaType<std::string>("std::string");
	qRegisterMetaType<AT_64>("AT_64");
	qRegisterMetaType<StoragePath>("StoragePath");
	qRegisterMetaType<ACQUISITION_MODE>("ACQUISITION_MODE");
	qRegisterMetaType<ACQUISITION_STATUS>("ACQUISITION_STATUS");
	qRegisterMetaType<BRILLOUIN_SETTINGS>("BRILLOUIN_SETTINGS");
	qRegisterMetaType<CAMERA_SETTINGS>("CAMERA_SETTINGS");
	qRegisterMetaType<CAMERA_SETTING>("CAMERA_SETTING");
	qRegisterMetaType<CAMERA_OPTIONS>("CAMERA_OPTIONS");
	qRegisterMetaType<std::vector<int>>("std::vector<int>");
	qRegisterMetaType<std::vector<double>>("std::vector<double>");
	qRegisterMetaType<std::vector<float>>("std::vector<float>");
	qRegisterMetaType<std::vector<unsigned short>>("std::vector<unsigned short>");
	qRegisterMetaType<std::vector<unsigned char>>("std::vector<unsigned char>");
	qRegisterMetaType<std::vector<unsigned int>>("std::vector<unsigned int>");
	qRegisterMetaType<std::vector<FLUORESCENCE_MODE>>("std::vector<FLUORESCENCE_MODE>");
	qRegisterMetaType<std::vector<POINT2>>("std::vector<POINT2>");
	qRegisterMetaType<std::vector<POINT3>>("std::vector<POINT3>");
	qRegisterMetaType<QSerialPort::SerialPortError>("QSerialPort::SerialPortError");
	qRegisterMetaType<IMAGE<unsigned char>*>("IMAGE<unsigned char>*");
	qRegisterMetaType<IMAGE<unsigned short>*>("IMAGE<unsigned short>*");
	qRegisterMetaType<CALIBRATION<unsigned char>*>("CALIBRATION<unsigned char>*");
	qRegisterMetaType<CALIBRATION<unsigned short>*>("CALIBRATION<unsigned short>*");
	qRegisterMetaType<ScanPreset>("ScanPreset");
	qRegisterMetaType<DeviceElement>("DeviceElement");
	qRegisterMetaType<SensorTemperature>("SensorTemperature");
	qRegisterMetaType<POINT3>("POINT3");
	qRegisterMetaType<POINT2>("POINT2");
	qRegisterMetaType<BOUNDS>("BOUNDS");
	qRegisterMetaType<QMouseEvent*>("QMouseEvent*");
	qRegisterMetaType<VOLTAGE2>("VOLTAGE2");
	qRegisterMetaType<ODT_MODE>("ODT_MODE");
	qRegisterMetaType<ODT_SETTING>("ODT_SETTING");
	qRegisterMetaType<ODT_SETTINGS>("ODT_SETTINGS");
	qRegisterMetaType<ODTIMAGE<unsigned char>*>("ODTIMAGE<unsigned char>*");
	qRegisterMetaType<ODTIMAGE<unsigned short>*>("ODTIMAGE<unsigned short>*");
	qRegisterMetaType<FLUOIMAGE<unsigned char>*>("FLUOIMAGE<unsigned char>*");
	qRegisterMetaType<FLUOIMAGE<unsigned short>*>("FLUOIMAGE<unsigned short>*");
	qRegisterMetaType<FLUORESCENCE_SETTINGS>("FLUORESCENCE_SETTINGS");
	qRegisterMetaType<FLUORESCENCE_MODE>("FLUORESCENCE_MODE");
	qRegisterMetaType<PLOT_SETTINGS*>("PLOT_SETTINGS*");
	qRegisterMetaType<PreviewBuffer<unsigned short>*>("PreviewBuffer<unsigned short>*");
	qRegisterMetaType<PreviewBuffer<unsigned char>*>("PreviewBuffer<unsigned char>*");
	qRegisterMetaType<unsigned char*>("unsigned char*");
	qRegisterMetaType<unsigned short*>("unsigned short*");
	qRegisterMetaType<bool*>("bool*");
	qRegisterMetaType<VoltageCalibrationData>("VoltageCalibrationData");
	qRegisterMetaType<ScaleCalibrationData>("ScaleCalibrationData");
	qRegisterMetaType<SCAN_ORDER>("SCAN_ORDER");
	
	// Set up icons
	m_icons.disconnected.addFile(":/BrillouinAcquisition/assets/00disconnected10px.png", QSize(10, 10));
	m_icons.disconnected.addFile(":/BrillouinAcquisition/assets/00disconnected16px.png", QSize(16, 16));
	m_icons.disconnected.addFile(":/BrillouinAcquisition/assets/00disconnected24px.png", QSize(24, 24));
	m_icons.disconnected.addFile(":/BrillouinAcquisition/assets/00disconnected32px.png", QSize(32, 32));

	m_icons.standby.addFile(":/BrillouinAcquisition/assets/01standby10px.png", QSize(10, 10));
	m_icons.standby.addFile(":/BrillouinAcquisition/assets/01standby16px.png", QSize(16, 16));
	m_icons.standby.addFile(":/BrillouinAcquisition/assets/01standby24px.png", QSize(24, 24));
	m_icons.standby.addFile(":/BrillouinAcquisition/assets/01standby32px.png", QSize(32, 32));

	m_icons.cooling.addFile(":/BrillouinAcquisition/assets/02cooling10px.png", QSize(10, 10));
	m_icons.cooling.addFile(":/BrillouinAcquisition/assets/02cooling16px.png", QSize(16, 16));
	m_icons.cooling.addFile(":/BrillouinAcquisition/assets/02cooling24px.png", QSize(24, 24));
	m_icons.cooling.addFile(":/BrillouinAcquisition/assets/02cooling32px.png", QSize(32, 32));

	m_icons.ready.addFile(":/BrillouinAcquisition/assets/03ready10px.png", QSize(10, 10));
	m_icons.ready.addFile(":/BrillouinAcquisition/assets/03ready16px.png", QSize(16, 16));
	m_icons.ready.addFile(":/BrillouinAcquisition/assets/03ready24px.png", QSize(24, 24));
	m_icons.ready.addFile(":/BrillouinAcquisition/assets/03ready32px.png", QSize(32, 32));

	m_icons.fluoBlue.addFile(":/BrillouinAcquisition/assets/04fluoBlue10px.png", QSize(10, 10));
	m_icons.fluoBlue.addFile(":/BrillouinAcquisition/assets/04fluoBlue16px.png", QSize(16, 16));
	m_icons.fluoBlue.addFile(":/BrillouinAcquisition/assets/04fluoBlue24px.png", QSize(24, 24));
	m_icons.fluoBlue.addFile(":/BrillouinAcquisition/assets/04fluoBlue32px.png", QSize(32, 32));

	m_icons.fluoGreen.addFile(":/BrillouinAcquisition/assets/05fluoGreen10px.png", QSize(10, 10));
	m_icons.fluoGreen.addFile(":/BrillouinAcquisition/assets/05fluoGreen16px.png", QSize(16, 16));
	m_icons.fluoGreen.addFile(":/BrillouinAcquisition/assets/05fluoGreen24px.png", QSize(24, 24));
	m_icons.fluoGreen.addFile(":/BrillouinAcquisition/assets/05fluoGreen32px.png", QSize(32, 32));

	m_icons.fluoRed.addFile(":/BrillouinAcquisition/assets/06fluoRed10px.png", QSize(10, 10));
	m_icons.fluoRed.addFile(":/BrillouinAcquisition/assets/06fluoRed16px.png", QSize(16, 16));
	m_icons.fluoRed.addFile(":/BrillouinAcquisition/assets/06fluoRed24px.png", QSize(24, 24));
	m_icons.fluoRed.addFile(":/BrillouinAcquisition/assets/06fluoRed32px.png", QSize(32, 32));

	m_icons.fluoBrightfield.addFile(":/BrillouinAcquisition/assets/07fluoBrightfield10px.png", QSize(10, 10));
	m_icons.fluoBrightfield.addFile(":/BrillouinAcquisition/assets/07fluoBrightfield16px.png", QSize(16, 16));
	m_icons.fluoBrightfield.addFile(":/BrillouinAcquisition/assets/07fluoBrightfield24px.png", QSize(24, 24));
	m_icons.fluoBrightfield.addFile(":/BrillouinAcquisition/assets/07fluoBrightfield32px.png", QSize(32, 32));

	ui->settingsWidget->setTabIcon(0, m_icons.disconnected);
	ui->settingsWidget->setTabIcon(1, m_icons.disconnected);
	ui->settingsWidget->setTabIcon(2, m_icons.disconnected);
	ui->settingsWidget->setTabIcon(3, m_icons.disconnected);
	ui->settingsWidget->setIconSize(QSize(10, 10));

	// Set icons for fluorescence
	QLabel *fluoBlueIcon = new QLabel(ui->fluoBlueIcon);
	fluoBlueIcon->setPixmap(m_icons.fluoBlue.pixmap(QSize(15, 15)));
	fluoBlueIcon->show();

	QLabel *fluoGreenIcon = new QLabel(ui->fluoGreenIcon);
	fluoGreenIcon->setPixmap(m_icons.fluoGreen.pixmap(QSize(15, 15)));
	fluoGreenIcon->show();

	QLabel *fluoRedIcon = new QLabel(ui->fluoRedIcon);
	fluoRedIcon->setPixmap(m_icons.fluoRed.pixmap(QSize(15, 15)));
	fluoRedIcon->show();

	QLabel *fluoBrightfieldIcon = new QLabel(ui->fluoBrightfieldIcon);
	fluoBrightfieldIcon->setPixmap(m_icons.fluoBrightfield.pixmap(QSize(15, 15)));
	fluoBrightfieldIcon->show();

	ui->actionEnable_Cooling->setEnabled(false);
	ui->autoscalePlot->setChecked(m_BrillouinPlot.autoscale);

	connection = QWidget::connect<void(converter::*)(PLOT_SETTINGS*, long long, long long, std::vector<unsigned char>)>(
		m_converter,
		&converter::s_converted,
		this,
		[this](PLOT_SETTINGS* plotSettings, long long dim_x, long long dim_y, std::vector<unsigned char> unpackedBuffer) { plot(plotSettings, dim_x, dim_y, unpackedBuffer); }
	);
	connection = QWidget::connect<void(converter::*)(PLOT_SETTINGS*, long long, long long, std::vector<unsigned short>)>(
		m_converter,
		&converter::s_converted,
		this,
		[this](PLOT_SETTINGS* plotSettings, long long dim_x, long long dim_y, std::vector<unsigned short> unpackedBuffer) { plot(plotSettings, dim_x, dim_y, unpackedBuffer); }
	);
	connection = QWidget::connect<void(converter::*)(PLOT_SETTINGS*, long long, long long, std::vector<double>)>(
		m_converter,
		&converter::s_converted,
		this,
		[this](PLOT_SETTINGS* plotSettings, long long dim_x, long long dim_y, std::vector<double> unpackedBuffer) { plot(plotSettings, dim_x, dim_y, unpackedBuffer); }
	);
	connection = QWidget::connect<void(converter::*)(PLOT_SETTINGS*, long long, long long, std::vector<float>)>(
		m_converter,
		&converter::s_converted,
		this,
		[this](PLOT_SETTINGS* plotSettings, long long dim_x, long long dim_y, std::vector<float> unpackedBuffer) { plot(plotSettings, dim_x, dim_y, unpackedBuffer); }
	);
	connection = QWidget::connect<void(converter::*)(PLOT_SETTINGS*, long long, long long, std::vector<int>)>(
		m_converter,
		&converter::s_converted,
		this,
		[this](PLOT_SETTINGS* plotSettings, long long dim_x, long long dim_y, std::vector<int> unpackedBuffer) { plot(plotSettings, dim_x, dim_y, unpackedBuffer); }
	);

	// start acquisition thread
	m_acquisitionThread.startWorker(m_acquisition);
	// start Brillouin thread
	m_acquisitionThread.startWorker(m_Brillouin);
	// start plotting thread
	m_plottingThread.startWorker(m_converter);

	// set up the QCPColorMap:
	m_BrillouinPlot = {
		ui->customplot,
		new QCPColorMap(ui->customplot->xAxis, ui->customplot->yAxis),
		{ 100, 300 },
		ui->rangeLower,
		ui->rangeUpper,
		[this](QCPRange range) {
			this->updateCLimRange(ui->rangeLower, ui->rangeUpper, range);
		},
		false,
		CustomGradientPreset::gpViridis
	};

	m_ODTPlot = {
		ui->customplot_brightfield,
		new QCPColorMap(ui->customplot_brightfield->xAxis, ui->customplot_brightfield->yAxis),
		{ 0, 100 },
		ui->rangeLowerODT,
		ui->rangeUpperODT,
		[this](QCPRange range) {
			this->updateCLimRange(ui->rangeLowerODT, ui->rangeUpperODT, range);
		},
		false,
		CustomGradientPreset::gpGrayscale
	};

	// set up the camera image plot
	BrillouinAcquisition::initializePlot(m_BrillouinPlot);
	BrillouinAcquisition::initializePlot(m_ODTPlot);

	connection = QWidget::connect(
		m_ODTPlot.plotHandle,
		&QCustomPlot::mousePress,
		this,
		[this](QMouseEvent* event) { plotClick(event); }
	);

	initializeODTVoltagePlot(ui->alignmentVoltagesODT);
	initializeODTVoltagePlot(ui->acquisitionVoltagesODT);

	// First read device settings then init devices
	readSettings();
	initCameraBrillouin();
	initScanControl();
	initCamera();

	// set up laser focus marker
	ui->addFocusMarker_brightfield->setIcon(m_icons.fluoBlue);
	ui->addFocusMarker_brightfield->setText("");
	initializeLaserPositionLocation();

	updateBrillouinSettings();
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

	// hide brightfield preview by default
	ui->brightfieldImage->hide();

	// set up scan direction radio button ids
	ui->buttonGroup->setId(ui->scanDirX0, 0);
	ui->buttonGroup->setId(ui->scanDirX1, 1);
	ui->buttonGroup->setId(ui->scanDirX2, 2);
	ui->buttonGroup_2->setId(ui->scanDirY0, 0);
	ui->buttonGroup_2->setId(ui->scanDirY1, 1);
	ui->buttonGroup_2->setId(ui->scanDirY2, 2);
	ui->buttonGroup_3->setId(ui->scanDirZ0, 0);
	ui->buttonGroup_3->setId(ui->scanDirZ1, 1);
	ui->buttonGroup_3->setId(ui->scanDirZ2, 2);
}

BrillouinAcquisition::~BrillouinAcquisition() {
	writeSettings();
	if (m_acquisition) {
		m_acquisition->deleteLater();
		m_acquisition = nullptr;
	}
	if (m_Brillouin) {
		m_Brillouin->deleteLater();
		m_Brillouin = nullptr;
	}
	if (m_ODT) {
		m_ODT->deleteLater();
		m_ODT = nullptr;
	}
	if (m_converter) {
		m_converter->deleteLater();
		m_converter = nullptr;
	}
	if (tableModel) {
		tableModel->deleteLater();
		tableModel = nullptr;
	}
	if (m_Fluorescence) {
		m_Fluorescence->deleteLater();
		m_Fluorescence = nullptr;
	}
	if (m_scaleCalibration) {
		m_scaleCalibration->deleteLater();
		m_scaleCalibration = nullptr;
	}
	if (m_voltageCalibration) {
		m_voltageCalibration->deleteLater();
		m_voltageCalibration = nullptr;
	}
	if (m_brightfieldCamera) {
		m_brightfieldCamera->deleteLater();
		m_brightfieldCamera = nullptr;
	}
	if (m_scanControl) {
		m_scanControl->deleteLater();
		m_scanControl = nullptr;
	}
	if (m_andor) {
		m_andor->deleteLater();
		m_andor = nullptr;
	}
	m_andorThread.exit();
	m_andorThread.wait();
	m_brightfieldCameraThread.exit();
	m_brightfieldCameraThread.wait();
	m_acquisitionThread.exit();
	m_acquisitionThread.wait();
	m_plottingThread.exit();
	m_plottingThread.wait();
	qInfo(logInfo()) << "BrillouinAcquisition closed.";
	delete ui;
}

void BrillouinAcquisition::closeEvent(QCloseEvent* event) {
	event->ignore();
	if (QMessageBox::Yes == confirmQuit()) {
		event->accept();
	}
}

void BrillouinAcquisition::on_actionQuit_triggered() {
	if (QMessageBox::Yes == confirmQuit()) {
		QApplication::quit();
	}
}

QMessageBox::StandardButton BrillouinAcquisition::confirmQuit() {
	// Directly quit in Debug mode
	#ifdef _DEBUG
		return QMessageBox::Yes;
	#else
		return QMessageBox::question(
			this,
			"Close BrillouinAcquisition?",
			"Do you really want to close BrillouinAcquisition?",
			QMessageBox::Yes | QMessageBox::No
		);
	#endif
}

void BrillouinAcquisition::plotClick(QMouseEvent* event) {
	auto position = event->pos();

	auto posX = m_ODTPlot.plotHandle->xAxis->pixelToCoord(position.x());
	auto posY = m_ODTPlot.plotHandle->yAxis->pixelToCoord(position.y());

	auto positionInPix = POINT2{ posX, posY };

	// If we currently select the new focus, don't move there
	if (m_locatePositionScanner) {
		m_scanControl->locatePositionScanner(positionInPix);
	} else {
		auto xRange = m_ODTPlot.plotHandle->xAxis->range();
		auto yRange = m_ODTPlot.plotHandle->yAxis->range();

		if (!xRange.contains(posX) || !yRange.contains(posY)) {
			return;
		}

		// Set laser focus to this position
		QMetaObject::invokeMethod(
			m_scanControl,
			[&m_scanControl = m_scanControl, positionInPix]() {
				m_scanControl->setPositionInPix(positionInPix);
			},
			Qt::QueuedConnection
		);
	}
}

void BrillouinAcquisition::showEvent(QShowEvent* event) {
	QWidget::showEvent(event);

	// connect microscope automatically
	QMetaObject::invokeMethod(
		m_scanControl,
		[&m_scanControl = m_scanControl]() {
			m_scanControl->connectDevice();
		},
		Qt::QueuedConnection
	);
}

void BrillouinAcquisition::setElement(DeviceElement element, double position) {
	QMetaObject::invokeMethod(
		m_scanControl,
		[&m_scanControl = m_scanControl, element, position]() {
			m_scanControl->setElement(element, position);
		},
		Qt::QueuedConnection
	);
}

void BrillouinAcquisition::on_autoscalePlot_stateChanged(int state) {
	m_BrillouinPlot.autoscale = (bool)state;
	ui->rangeLower->setDisabled(state);
	ui->rangeUpper->setDisabled(state);
}

void BrillouinAcquisition::on_autoscalePlot_brightfield_stateChanged(int state) {
	m_ODTPlot.autoscale = (bool)state;
	ui->rangeLowerODT->setDisabled(state);
	ui->rangeUpperODT->setDisabled(state);
}

void BrillouinAcquisition::setPreset(ScanPreset preset) {
	QMetaObject::invokeMethod(
		m_scanControl,
		[&m_scanControl = m_scanControl, preset]() {
			m_scanControl->setPreset(preset);
		},
		Qt::QueuedConnection);
}

void BrillouinAcquisition::cameraOptionsChanged(const CAMERA_OPTIONS& options) {
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

void BrillouinAcquisition::cameraODTOptionsChanged(const CAMERA_OPTIONS& options) {
	m_cameraOptionsODT.exposureTimeLimits = options.exposureTimeLimits;

	m_cameraOptionsODT.ROIHeightLimits = options.ROIHeightLimits;
	m_cameraOptionsODT.ROIWidthLimits = options.ROIWidthLimits;

	// Adjust plotting range only when neither preview nor acquisition are running
	if (!(m_brightfieldCamera->m_isPreviewRunning || m_brightfieldCamera->m_isAcquisitionRunning)) {
		m_ODTPlot.plotHandle->xAxis->setRange(QCPRange(1, options.ROIWidthLimits[1]));
		m_ODTPlot.plotHandle->yAxis->setRange(QCPRange(1, options.ROIHeightLimits[1]));

		ui->ROIHeightODT->setValue(options.ROIHeightLimits[1]);
		ui->ROITopODT->setValue(0);
		ui->ROIWidthODT->setValue(options.ROIWidthLimits[1]);
		ui->ROILeftODT->setValue(0);

	}

	ui->ROIHeightODT->setMinimum(options.ROIHeightLimits[0]);
	ui->ROIHeightODT->setMaximum(options.ROIHeightLimits[1]);
	ui->ROITopODT->setMinimum(0);
	ui->ROITopODT->setMaximum(options.ROIHeightLimits[1]);
	ui->ROIWidthODT->setMinimum(options.ROIWidthLimits[0]);
	ui->ROIWidthODT->setMaximum(options.ROIWidthLimits[1]);
	ui->ROILeftODT->setMinimum(0);
	ui->ROILeftODT->setMaximum(options.ROIWidthLimits[1]);

	// block signals to not trigger setting a new value
	const QSignalBlocker blocker(ui->exposureTimeODT);
	ui->exposureTimeODT->setMinimum(m_cameraOptionsODT.exposureTimeLimits[0]);
	ui->exposureTimeODT->setMaximum(m_cameraOptionsODT.exposureTimeLimits[1]);

	ui->fluoBlueExposure->setMinimum(1e3*m_cameraOptionsODT.exposureTimeLimits[0]);
	ui->fluoBlueExposure->setMaximum(1e3*m_cameraOptionsODT.exposureTimeLimits[1]);
	ui->fluoGreenExposure->setMinimum(1e3*m_cameraOptionsODT.exposureTimeLimits[0]);
	ui->fluoGreenExposure->setMaximum(1e3*m_cameraOptionsODT.exposureTimeLimits[1]);
	ui->fluoRedExposure->setMinimum(1e3*m_cameraOptionsODT.exposureTimeLimits[0]);
	ui->fluoRedExposure->setMaximum(1e3*m_cameraOptionsODT.exposureTimeLimits[1]);
	ui->fluoBrightfieldExposure->setMinimum(1e3*m_cameraOptionsODT.exposureTimeLimits[0]);
	ui->fluoBrightfieldExposure->setMaximum(1e3*m_cameraOptionsODT.exposureTimeLimits[1]);

	addListToComboBox(ui->pixelEncodingODT, options.pixelEncodings);
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
		const QSignalBlocker blocker(ui->setPositionX);
		ui->setPositionX->setValue(position.x);
	}
	if (!ui->setPositionY->hasFocus()) {
		const QSignalBlocker blocker(ui->setPositionY);
		ui->setPositionY->setValue(position.y);
	}
	if (!ui->setPositionZ->hasFocus()) {
		const QSignalBlocker blocker(ui->setPositionZ);
		ui->setPositionZ->setValue(position.z);
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

void BrillouinAcquisition::addListToComboBox(QComboBox* box, const std::vector<std::wstring>& list) {
	// Check whether we have to do anything
	std::vector< std::wstring > currentList(box->count());
	for (gsl::index i{ 0 }; i < currentList.size(); i++) {
		currentList[i] = box->itemText(i).toStdWString();
	}
	if (currentList == list) {
		return;
	}
	const QSignalBlocker blocker(box);
	box->clear();
	for (auto &item : list) {
		box->addItem(QString::fromStdWString(item));
	}
}

void BrillouinAcquisition::cameraSettingsChanged(const CAMERA_SETTINGS& settings) {
	ui->exposureTime->setValue(settings.exposureTime);
	ui->frameCount->setValue(settings.frameCount);
	//ui->ROILeft->setValue(settings.roi.left);
	//ui->ROIWidth->setValue(settings.roi.width);
	//ui->ROITop->setValue(settings.roi.top);
	//ui->ROIHeight->setValue(settings.roi.height);

	ui->triggerMode->setCurrentText(QString::fromStdWString(settings.readout.triggerMode));
	ui->binning->setCurrentText(QString::fromStdWString(settings.roi.binning));

	ui->pixelReadoutRate->setCurrentText(QString::fromStdWString(settings.readout.pixelReadoutRate));
	ui->cycleMode->setCurrentText(QString::fromStdWString(settings.readout.cycleMode));
	ui->preAmpGain->setCurrentText(QString::fromStdWString(settings.readout.preAmpGain));
	ui->pixelEncoding->setCurrentText(QString::fromStdWString(settings.readout.pixelEncoding));
}

void BrillouinAcquisition::cameraODTSettingsChanged(const CAMERA_SETTINGS& settings) {
	const QSignalBlocker blocker1(ui->exposureTimeODT);
	ui->exposureTimeODT->setValue(settings.exposureTime);

	const QSignalBlocker blocker2(ui->gainODT);
	ui->gainODT->setValue(settings.gain);

	//ui->frameCount->setValue(settings.frameCount);
	ui->ROILeftODT->setValue(settings.roi.left);
	ui->ROIWidthODT->setValue(settings.roi.width_physical);
	ui->ROITopODT->setValue(settings.roi.top);
	ui->ROIHeightODT->setValue(settings.roi.height_physical);

	ui->pixelEncodingODT->setCurrentText(QString::fromStdWString(settings.readout.pixelEncoding));
}

void BrillouinAcquisition::updateODTCameraSettings(const CAMERA_SETTINGS& settings) {
	const QSignalBlocker blocker1(ui->exposureTimeCameraODT);
	ui->exposureTimeCameraODT->setValue(settings.exposureTime);

	const QSignalBlocker blocker2(ui->gainCameraODT);
	ui->gainCameraODT->setValue(settings.gain);
}

void BrillouinAcquisition::sensorTemperatureChanged(const SensorTemperature& sensorTemperature) {
	ui->sensorTemp->setValue(sensorTemperature.temperature);
	if (sensorTemperature.status == enCameraTemperatureStatus::COOLER_OFF ||
		sensorTemperature.status == enCameraTemperatureStatus::FAULT || sensorTemperature.status == enCameraTemperatureStatus::DRIFT) {
		ui->settingsWidget->setTabIcon(0, m_icons.standby);
	} else if (sensorTemperature.status == enCameraTemperatureStatus::COOLING
		|| sensorTemperature.status == enCameraTemperatureStatus::NOT_STABILISED) {
		ui->settingsWidget->setTabIcon(0, m_icons.cooling);
	} else if (sensorTemperature.status == enCameraTemperatureStatus::STABILISED) {
		ui->settingsWidget->setTabIcon(0, m_icons.ready);
	} else {
		ui->settingsWidget->setTabIcon(0, m_icons.disconnected);
	}
}

void BrillouinAcquisition::initializeODTVoltagePlot(QCustomPlot *plot) {
	plot->setBackground(QColor(240, 240, 240, 255));
	plot->axisRect()->setBackground(Qt::white);
	//plot->xAxis->setLabel("Ux [V]");
	//plot->yAxis->setLabel("Uy [V]");

	plot->axisRect()->setupFullAxesBox(true);

	plot->axisRect()->setMaximumSize(210, 210); // make bottom right axis rect size fixed 150x150
	plot->axisRect()->setMinimumSize(210, 210);

	plot->addGraph();
	plot->graph(0)->setLineStyle(QCPGraph::LineStyle::lsNone);
	plot->graph(0)->setScatterStyle(QCPScatterStyle::ScatterShape::ssCircle);
	plot->addGraph();
	plot->graph(1)->setLineStyle(QCPGraph::LineStyle::lsNone);
	plot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, Qt::red, 10));
	plot->replot();
}

/*
* ODT signals
*/

void BrillouinAcquisition::on_alignmentUR_ODT_valueChanged(double voltage) {
	m_ODT->setSettings(ODT_MODE::ALGN, ODT_SETTING::VOLTAGE, voltage);
}

void BrillouinAcquisition::on_alignmentNumber_ODT_valueChanged(int number) {
	QMetaObject::invokeMethod(
		m_ODT,
		[&m_ODT = m_ODT, number]() {
			m_ODT->setSettings(ODT_MODE::ALGN, ODT_SETTING::NRPOINTS, (double)number);
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::on_alignmentRate_ODT_valueChanged(double rate) {
	QMetaObject::invokeMethod(
		m_ODT,
		[&m_ODT = m_ODT, rate]() {
			m_ODT->setSettings(ODT_MODE::ALGN, ODT_SETTING::SCANRATE, rate);
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::on_alignmentStartODT_clicked() {
	QMetaObject::invokeMethod(
		m_ODT,
		[&m_ODT = m_ODT]() {
			m_ODT->startAlignment();
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::on_alignmentCenterODT_clicked() {
	QMetaObject::invokeMethod(
		m_ODT,
		[&m_ODT = m_ODT]() {
			m_ODT->centerAlignment();
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::on_acquisitionUR_ODT_valueChanged(double voltage) {
	m_ODT->setSettings(ODT_MODE::ACQ, ODT_SETTING::VOLTAGE, voltage);
}

void BrillouinAcquisition::on_acquisitionNumber_ODT_valueChanged(int number) {
	m_ODT->setSettings(ODT_MODE::ACQ, ODT_SETTING::NRPOINTS, number);
}

void BrillouinAcquisition::on_acquisitionRate_ODT_valueChanged(double rate) {
	m_ODT->setSettings(ODT_MODE::ACQ, ODT_SETTING::SCANRATE, rate);
}

void BrillouinAcquisition::on_acquisitionStartODT_clicked() {
	if (m_ODT->getStatus() < ACQUISITION_STATUS::STARTED) {
		QMetaObject::invokeMethod(
			m_ODT,
			[&m_ODT = m_ODT]() {
				m_ODT->startRepetitions();
			},
			Qt::AutoConnection
		);
	} else {
		m_ODT->m_abort = true;
	}
}

void BrillouinAcquisition::on_exposureTimeODT_valueChanged(double exposureTime) {
	QMetaObject::invokeMethod(
		m_brightfieldCamera,
		[&m_brightfieldCamera = m_brightfieldCamera, exposureTime]() {
			m_brightfieldCamera->setSetting(CAMERA_SETTING::EXPOSURE, exposureTime);
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::on_gainODT_valueChanged(double gain) {
	QMetaObject::invokeMethod(
		m_brightfieldCamera,
		[&m_brightfieldCamera = m_brightfieldCamera, gain]() {
			m_brightfieldCamera->setSetting(CAMERA_SETTING::GAIN, gain);
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::on_exposureTimeCameraODT_valueChanged(double exposureTime) {
	QMetaObject::invokeMethod(
		m_ODT,
		[&m_ODT = m_ODT, exposureTime]() {
			m_ODT->setCameraSetting(CAMERA_SETTING::EXPOSURE, exposureTime);
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::on_gainCameraODT_valueChanged(double gain) {
	QMetaObject::invokeMethod(
		m_ODT,
		[&m_ODT = m_ODT, gain]() {
			m_ODT->setCameraSetting(CAMERA_SETTING::GAIN, gain);
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::on_pixelEncodingODT_currentIndexChanged(const QString& text) {
	auto encoding = text.toStdWString();
	QMetaObject::invokeMethod(
		m_brightfieldCamera,
		[&m_brightfieldCamera = m_brightfieldCamera, encoding]() {
			m_brightfieldCamera->setSetting(CAMERA_SETTING::ENCODING, encoding);
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::on_camera_displayMode_currentIndexChanged(const QString & text) {
	if (text == "Intensity") {
		m_ODTPlot.mode = DISPLAY_MODE::INTENSITY;
		m_ODTPlot.gradient = CustomGradientPreset::gpGrayscale;
	} else if (text == "Spectrum") {
		m_ODTPlot.mode = DISPLAY_MODE::SPECTRUM;
		m_ODTPlot.gradient = CustomGradientPreset::gpInferno;
	} else if (text == "Phase") {
		m_ODTPlot.mode = DISPLAY_MODE::PHASE;
		m_ODTPlot.gradient = CustomGradientPreset::gpInferno;
	} else {
		m_ODTPlot.mode = DISPLAY_MODE::INTENSITY;
		m_ODTPlot.gradient = CustomGradientPreset::gpGrayscale;
	}
	applyGradient(m_ODTPlot);
}

void BrillouinAcquisition::on_setBackground_clicked() {
	QMetaObject::invokeMethod(
		m_converter,
		[&m_converter = m_converter]() {
			m_converter->updateBackground();
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::on_acquisitionStartFluorescence_clicked() {
	if (m_Fluorescence->getStatus() < ACQUISITION_STATUS::STARTED) {
		QMetaObject::invokeMethod(
			m_Fluorescence,
			[&m_Fluorescence = m_Fluorescence]() {
				m_Fluorescence->startRepetitions();
			},
			Qt::AutoConnection
		);
	} else {
		m_Fluorescence->m_abort = true;
	}
}

void BrillouinAcquisition::on_fluoBlueStart_clicked() {
	if (m_Fluorescence->getStatus() < ACQUISITION_STATUS::STARTED) {
		QMetaObject::invokeMethod(
			m_Fluorescence,
			[&m_Fluorescence = m_Fluorescence]() {
				m_Fluorescence->startRepetitions({ FLUORESCENCE_MODE::BLUE });
			},
			Qt::AutoConnection
		);
	}
	else {
		m_Fluorescence->m_abort = true;
	}
}

void BrillouinAcquisition::on_fluoGreenStart_clicked() {
	if (m_Fluorescence->getStatus() < ACQUISITION_STATUS::STARTED) {
		QMetaObject::invokeMethod(
			m_Fluorescence,
			[&m_Fluorescence = m_Fluorescence]() {
				m_Fluorescence->startRepetitions({ FLUORESCENCE_MODE::GREEN });
			},
			Qt::AutoConnection
		);
	}
	else {
		m_Fluorescence->m_abort = true;
	}
}

void BrillouinAcquisition::on_fluoRedStart_clicked() {
	if (m_Fluorescence->getStatus() < ACQUISITION_STATUS::STARTED) {
		QMetaObject::invokeMethod(
			m_Fluorescence,
			[&m_Fluorescence = m_Fluorescence]() {
				m_Fluorescence->startRepetitions({ FLUORESCENCE_MODE::RED });
			},
			Qt::AutoConnection
		);
	}
	else {
		m_Fluorescence->m_abort = true;
	}
}

void BrillouinAcquisition::on_fluoBrightfieldStart_clicked() {
	if (m_Fluorescence->getStatus() < ACQUISITION_STATUS::STARTED) {
		QMetaObject::invokeMethod(
			m_Fluorescence,
			[&m_Fluorescence = m_Fluorescence]() {
				m_Fluorescence->startRepetitions({ FLUORESCENCE_MODE::BRIGHTFIELD });
			},
			Qt::AutoConnection
		);
	}
	else {
		m_Fluorescence->m_abort = true;
	}
}

void BrillouinAcquisition::on_fluoBluePreview_clicked() {
	QMetaObject::invokeMethod(
		m_Fluorescence,
		[&m_Fluorescence = m_Fluorescence]() {
			m_Fluorescence->startStopPreview({ FLUORESCENCE_MODE::BLUE });
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::on_fluoGreenPreview_clicked() {
	QMetaObject::invokeMethod(
		m_Fluorescence,
		[&m_Fluorescence = m_Fluorescence]() {
			m_Fluorescence->startStopPreview({ FLUORESCENCE_MODE::GREEN });
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::on_fluoRedPreview_clicked() {
	QMetaObject::invokeMethod(
		m_Fluorescence,
		[&m_Fluorescence = m_Fluorescence]() {
			m_Fluorescence->startStopPreview({ FLUORESCENCE_MODE::RED });
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::on_fluoBrightfieldPreview_clicked() {
	QMetaObject::invokeMethod(
		m_Fluorescence,
		[&m_Fluorescence = m_Fluorescence]() {
			m_Fluorescence->startStopPreview({ FLUORESCENCE_MODE::BRIGHTFIELD });
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::on_fluoBlueCheckbox_stateChanged(int enabled) {
	m_Fluorescence->setChannel(FLUORESCENCE_MODE::BLUE, enabled);
}

void BrillouinAcquisition::on_fluoGreenCheckbox_stateChanged(int enabled) {
	m_Fluorescence->setChannel(FLUORESCENCE_MODE::GREEN, enabled);
}

void BrillouinAcquisition::on_fluoRedCheckbox_stateChanged(int enabled) {
	m_Fluorescence->setChannel(FLUORESCENCE_MODE::RED, enabled);
}

void BrillouinAcquisition::on_fluoBrightfieldCheckbox_stateChanged(int enabled) {
	m_Fluorescence->setChannel(FLUORESCENCE_MODE::BRIGHTFIELD, enabled);
}

void BrillouinAcquisition::on_fluoBlueExposure_valueChanged(int exposure) {
	m_Fluorescence->setExposure(FLUORESCENCE_MODE::BLUE, exposure);
}

void BrillouinAcquisition::on_fluoGreenExposure_valueChanged(int exposure) {
	m_Fluorescence->setExposure(FLUORESCENCE_MODE::GREEN, exposure);
}

void BrillouinAcquisition::on_fluoRedExposure_valueChanged(int exposure) {
	m_Fluorescence->setExposure(FLUORESCENCE_MODE::RED, exposure);
}

void BrillouinAcquisition::on_fluoBrightfieldExposure_valueChanged(int exposure) {
	m_Fluorescence->setExposure(FLUORESCENCE_MODE::BRIGHTFIELD, exposure);
}

void BrillouinAcquisition::on_fluoBlueGain_valueChanged(double gain) {
	m_Fluorescence->setGain(FLUORESCENCE_MODE::BLUE, gain);
}

void BrillouinAcquisition::on_fluoGreenGain_valueChanged(double gain) {
	m_Fluorescence->setGain(FLUORESCENCE_MODE::GREEN, gain);
}

void BrillouinAcquisition::on_fluoRedGain_valueChanged(double gain) {
	m_Fluorescence->setGain(FLUORESCENCE_MODE::RED, gain);
}

void BrillouinAcquisition::on_fluoBrightfieldGain_valueChanged(double gain) {
	m_Fluorescence->setGain(FLUORESCENCE_MODE::BRIGHTFIELD, gain);
}

void BrillouinAcquisition::updateFluorescenceSettings(const FLUORESCENCE_SETTINGS& settings) {
	bool disableStart{ true };
	if (settings.blue.enabled || settings.red.enabled || settings.green.enabled || settings.brightfield.enabled) {
		disableStart = false;
	}
	ui->acquisitionStartFluorescence->setDisabled(disableStart);
	ui->fluoBlueCheckbox->setChecked(settings.blue.enabled);
	ui->fluoGreenCheckbox->setChecked(settings.green.enabled);
	ui->fluoRedCheckbox->setChecked(settings.red.enabled);
	ui->fluoBrightfieldCheckbox->setChecked(settings.brightfield.enabled);
	ui->fluoBlueExposure->setValue(settings.blue.exposure);
	ui->fluoGreenExposure->setValue(settings.green.exposure);
	ui->fluoRedExposure->setValue(settings.red.exposure);
	ui->fluoBrightfieldExposure->setValue(settings.brightfield.exposure);
	ui->fluoBlueGain->setValue(settings.blue.gain);
	ui->fluoGreenGain->setValue(settings.green.gain);
	ui->fluoRedGain->setValue(settings.red.gain);
	ui->fluoBrightfieldGain->setValue(settings.brightfield.gain);
}

void BrillouinAcquisition::showEnabledModes(ACQUISITION_MODE modes) {
	m_enabledModes = modes;
	/*
	 * Handle Brillouin and fluorescence mode
	 *
	 * If either Brillouin or fluorescence mode is enabled, disable ODT controls (enable otherwise).
	 */
	bool BrillouinMode = (bool)(m_enabledModes & ACQUISITION_MODE::BRILLOUIN);
	bool FluorescenceMode = (bool)(m_enabledModes & ACQUISITION_MODE::FLUORESCENCE);

	if (BrillouinMode || FluorescenceMode) {
		ui->acquisitionStartODT->setEnabled(false);
		ui->alignmentStartODT->setEnabled(false);
		ui->alignmentCenterODT->setEnabled(false);
	} else {
		ui->acquisitionStartODT->setEnabled(true);
		ui->alignmentStartODT->setEnabled(true);
		ui->alignmentCenterODT->setEnabled(true);
	}

	/*
	* Handle ODT mode
	*
	* If ODT mode is enabled, disable Brillouin and Fluorescence controls (enable otherwise).
	*/
	bool ODTMode = (bool)(m_enabledModes & ACQUISITION_MODE::ODT);

	if (ODTMode) {
		ui->BrillouinStart->setEnabled(false);
	} else {
		ui->BrillouinStart->setEnabled(true);
	}
}

void BrillouinAcquisition::showBrillouinStatus(ACQUISITION_STATUS status) {
	QString string;
	bool running{ false };
	switch (status) {
		case ACQUISITION_STATUS::ABORTED:
			string = "Acquisition aborted.";
			ui->progressBar->setValue(0);
			ui->BrillouinStart->setText("Start");
			break;
		case ACQUISITION_STATUS::FINISHED:
			string = "Acquisition finished.";
			ui->progressBar->setValue(100);
			ui->BrillouinStart->setText("Start");
			break;
		case ACQUISITION_STATUS::STARTED:
			string = "Acquisition started.";
			ui->progressBar->setValue(0);
			[[fallthrough]];
		case ACQUISITION_STATUS::RUNNING:
			ui->BrillouinStart->setText("Cancel");
			running = true;
			break;
		case ACQUISITION_STATUS::WAITFORREPETITION:
			ui->BrillouinStart->setText("Stop");
			running = true;
			break;
		case ACQUISITION_STATUS::STOPPED:
			ui->BrillouinStart->setText("Start");
			break;
		default:
			ui->BrillouinStart->setText("Start");
			break;
	}
	ui->progressBar->setFormat(string);

	ui->actionOpen_Acquisition->setDisabled(running);
	ui->actionNew_Acquisition->setDisabled(running);
	ui->actionClose_Acquisition->setDisabled(running);

	ui->startX->setDisabled(running);
	ui->startY->setDisabled(running);
	ui->startZ->setDisabled(running);
	ui->endX->setDisabled(running);
	ui->endY->setDisabled(running);
	ui->endZ->setDisabled(running);
	ui->stepsX->setDisabled(running);
	ui->stepsY->setDisabled(running);
	ui->stepsZ->setDisabled(running);
	ui->camera_playPause->setDisabled(running);
	ui->camera_singleShot->setDisabled(running);
	ui->setHome->setDisabled(running);
	ui->setPositionX->setDisabled(running);
	ui->setPositionY->setDisabled(running);
	ui->setPositionZ->setDisabled(running);

	ui->postCalibration->setDisabled(running);
	ui->preCalibration->setDisabled(running);
	ui->conCalibration->setDisabled(running);
	ui->conCalibrationInterval->setDisabled(running);
	ui->sampleSelection->setDisabled(running);
	ui->nrCalibrationImages->setDisabled(running);
	ui->calibrationExposureTime->setDisabled(running);
	ui->repetitionInterval->setDisabled(running);
	ui->repetitionCount->setDisabled(running);
}

void BrillouinAcquisition::showBrillouinProgress(double progress, int seconds) {
	ui->progressBar->setValue(progress);

	QString string;
	QString timeString = formatSeconds(seconds);
	string.sprintf("%02.1f %% finished, ", progress);
	string += timeString;
	string += " remaining.";
	ui->progressBar->setFormat(string);
}

void BrillouinAcquisition::showODTStatus(ACQUISITION_STATUS status) {
	QString string;
	if (status == ACQUISITION_STATUS::ABORTED) {
		string = "Acquisition aborted.";
		ui->acquisitionProgress_ODT->setValue(0);
	}
	else if (status == ACQUISITION_STATUS::FINISHED) {
		string = "Acquisition finished.";
		ui->acquisitionProgress_ODT->setValue(100);
	}
	else if (status == ACQUISITION_STATUS::STARTED) {
		string = "Acquisition started.";
		ui->acquisitionProgress_ODT->setValue(0);
	}
	ui->acquisitionProgress_ODT->setFormat(string);

	bool running{ false };
	if (status == ACQUISITION_STATUS::RUNNING || status == ACQUISITION_STATUS::STARTED) {
		ui->acquisitionStartODT->setText("Cancel");
		running = true;
	} else {
		ui->acquisitionStartODT->setText("Start");
	}

	ui->actionOpen_Acquisition->setDisabled(running);
	ui->actionNew_Acquisition->setDisabled(running);
	ui->actionClose_Acquisition->setDisabled(running);

	ui->alignmentStartODT->setDisabled(running);
	ui->acquisitionUR_ODT->setDisabled(running);
	ui->acquisitionNumber_ODT->setDisabled(running);
	ui->acquisitionRate_ODT->setDisabled(running);

	if (status == ACQUISITION_STATUS::ALIGNING) {
		ui->alignmentStartODT->setText("Stop");
	} else {
		ui->alignmentStartODT->setText("Start");
	}

	if (status > ACQUISITION_STATUS::STOPPED) {
		ui->alignmentCenterODT->setEnabled(false);
	} else {
		ui->alignmentCenterODT->setEnabled(true);
	}
}

void BrillouinAcquisition::showODTProgress(double progress, int seconds) {
	ui->acquisitionProgress_ODT->setValue(progress);

	QString string;
	QString timeString = formatSeconds(seconds);
	string.sprintf("%02.1f %% finished, ", progress);
	string += timeString;
	string += " remaining.";
	ui->acquisitionProgress_ODT->setFormat(string);
}

void BrillouinAcquisition::showFluorescenceStatus(ACQUISITION_STATUS status) {
	QString string;
	if (status == ACQUISITION_STATUS::ABORTED) {
		string = "Acquisition aborted.";
		ui->fluoProgress->setValue(0);
	} else if (status == ACQUISITION_STATUS::FINISHED) {
		string = "Acquisition finished.";
		ui->fluoProgress->setValue(100);
	} else if (status == ACQUISITION_STATUS::STARTED) {
		string = "Acquisition started.";
		ui->fluoProgress->setValue(0);
	}
	ui->fluoProgress->setFormat(string);

	bool running{ false };
	if (status == ACQUISITION_STATUS::RUNNING || status == ACQUISITION_STATUS::STARTED) {
		ui->acquisitionStartFluorescence->setText("Cancel");
		running = true;
	} else {
		ui->acquisitionStartFluorescence->setText("Acquire All");
	}
	//startBrightfieldPreview(running);

	ui->fluoBlueStart->setDisabled(running);
	ui->fluoGreenStart->setDisabled(running);
	ui->fluoRedStart->setDisabled(running);
	ui->fluoBrightfieldStart->setDisabled(running);

	// reset all preview buttons
	ui->fluoBluePreview->setText("Preview");
	ui->fluoGreenPreview->setText("Preview");
	ui->fluoRedPreview->setText("Preview");
	ui->fluoBrightfieldPreview->setText("Preview");

	ui->fluoBluePreview->setDisabled(running);
	ui->fluoGreenPreview->setDisabled(running);
	ui->fluoRedPreview->setDisabled(running);
	ui->fluoBrightfieldPreview->setDisabled(running);

	ui->fluoBlueCheckbox->setDisabled(running);
	ui->fluoGreenCheckbox->setDisabled(running);
	ui->fluoRedCheckbox->setDisabled(running);
	ui->fluoBrightfieldCheckbox->setDisabled(running);
	ui->fluoBlueExposure->setDisabled(running);
	ui->fluoGreenExposure->setDisabled(running);
	ui->fluoRedExposure->setDisabled(running);
	ui->fluoBrightfieldExposure->setDisabled(running);
	ui->fluoBlueGain->setDisabled(running);
	ui->fluoGreenGain->setDisabled(running);
	ui->fluoRedGain->setDisabled(running);
	ui->fluoBrightfieldGain->setDisabled(running);
}

void BrillouinAcquisition::showFluorescenceProgress(double progress, int seconds) {
	ui->fluoProgress->setValue(progress);

	QString string;
	QString timeString = formatSeconds(seconds);
	string.sprintf("%02.1f %% finished, ", progress);
	string += timeString;
	string += " remaining.";
	ui->fluoProgress->setFormat(string);
}

QString BrillouinAcquisition::formatSeconds(int seconds) {
	QString string;
	if (seconds > 3600) {
		int hours = floor((double)seconds / 3600);
		int minutes = floor((seconds - hours * 3600) / 60);
		string.sprintf("%02.0f:%02.0f hours", (double)hours, (double)minutes);
	}
	else if (seconds > 60) {
		int minutes = floor(seconds / 60);
		seconds = floor(seconds - minutes * 60);
		string.sprintf("%02.0f:%02.0f minutes", (double)minutes, (double)seconds);
	}
	else {
		string.sprintf("%2.0f seconds", (double)seconds);
	}
	return string;
}

void BrillouinAcquisition::plotODTVoltages(const ODT_SETTINGS& settings, const ODT_MODE& mode) {
	QCustomPlot *plot = nullptr;
	switch (mode) {
		case ODT_MODE::ALGN: {
			plot = ui->alignmentVoltagesODT;
			const QSignalBlocker blocker1(ui->alignmentUR_ODT);
			const QSignalBlocker blocker2(ui->alignmentNumber_ODT);
			const QSignalBlocker blocker3(ui->alignmentRate_ODT);
			ui->alignmentUR_ODT->setValue(settings.radialVoltage);
			ui->alignmentNumber_ODT->setValue(settings.numberPoints);
			ui->alignmentRate_ODT->setValue(settings.scanRate);
			break;
		}
		case ODT_MODE::ACQ: {
			plot = ui->acquisitionVoltagesODT;
			const QSignalBlocker blocker1(ui->acquisitionUR_ODT);
			const QSignalBlocker blocker2(ui->acquisitionNumber_ODT);
			const QSignalBlocker blocker3(ui->acquisitionRate_ODT);
			ui->acquisitionUR_ODT->setValue(settings.radialVoltage);
			ui->acquisitionNumber_ODT->setValue(settings.numberPoints);
			ui->acquisitionRate_ODT->setValue(settings.scanRate);
			break;
		}
		default:
			return;
	}

	std::vector<VOLTAGE2> voltages = settings.voltages;
	QVector<double> Ux(voltages.size()), Uy(voltages.size());
	for (gsl::index index{ 0 }; index < voltages.size(); ++index) {
		Ux[index] = voltages[index].Ux;
		Uy[index] = voltages[index].Uy;
	}

	plot->graph(0)->setData(Ux, Uy);
	// scale the axis appropriately
	plot->xAxis->setRange(-1.1*settings.radialVoltage, 1.1*settings.radialVoltage);
	plot->yAxis->setRange(-1.1*settings.radialVoltage, 1.1*settings.radialVoltage);
	
	// set the aspect ratio
	//plot->yAxis->setScaleRatio(plot->xAxis, 1.0); // somehow makes it worse
	plot->replot();
}

void BrillouinAcquisition::plotODTVoltage(const VOLTAGE2& voltage, const ODT_MODE& mode) {
	QCustomPlot *plot;
	switch (mode) {
		case ODT_MODE::ALGN:
			plot = ui->alignmentVoltagesODT;
			break;
		case ODT_MODE::ACQ:
			plot = ui->acquisitionVoltagesODT;
			break;
		default:
			return;
	}
	QVector<double> Ux{ voltage.Ux }, Uy{ voltage.Uy };
	plot->graph(1)->setData(Ux, Uy);
	plot->replot();
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

	auto connection = QWidget::connect<void(QCPColorMap::*)(const QCPRange &)>(
		plotSettings.colorMap,
		&QCPColorMap::dataRangeChanged,
		this,
		[this, plotSettings](QCPRange newRange) { (plotSettings.dataRangeCallback)(newRange); }
	);

	// set the color gradient of the color map to one of the presets:
	applyGradient(plotSettings);

	plotSettings.colorMap->setDataRange(plotSettings.cLim);

	(plotSettings.dataRangeCallback)(plotSettings.cLim);
	// rescale the data dimension (color) such that all data points lie in the span visualized by the color gradient:
	if (plotSettings.autoscale) {
		plotSettings.colorMap->rescaleDataRange(true);
		plotSettings.cLim = plotSettings.colorMap->dataRange();
		(plotSettings.dataRangeCallback)(plotSettings.cLim);
	}

	// make sure the axis rect and color scale synchronize their bottom and top margins (so they line up):
	QCPMarginGroup *marginGroup = new QCPMarginGroup(plotSettings.plotHandle);
	plotSettings.plotHandle->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
	colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);

	// rescale the key (x) and value (y) axes so the whole color map is visible:
	plotSettings.plotHandle->rescaleAxes();
}

void BrillouinAcquisition::applyGradient(const PLOT_SETTINGS& plotSettings) {
	QCPColorGradient gradient = QCPColorGradient();
	setColormap(&gradient, plotSettings.gradient);
	plotSettings.colorMap->setGradient(gradient);
}

void BrillouinAcquisition::initializeLaserPositionLocation() {
	// If the scanControl supports capability LaserScanner, there is no need to set the laser position manually.
	if (m_scanControl != nullptr && m_scanControl->supportsCapability(Capabilities::LaserScanner)) {
		ui->addFocusMarker_brightfield->hide();
	} else {
		ui->addFocusMarker_brightfield->show();
	}
}

void BrillouinAcquisition::on_addFocusMarker_brightfield_clicked() {
	// If the scanControl supports capability LaserScanner, there is no need to set the laser position manually.
	if (m_scanControl != nullptr && m_scanControl->supportsCapability(Capabilities::LaserScanner)) {
		return;
	}

	m_locatePositionScanner = !m_locatePositionScanner;

	if (!m_locatePositionScanner) {
		ui->addFocusMarker_brightfield->setIcon(m_icons.fluoBlue);
		ui->addFocusMarker_brightfield->setText("");
	} else {
		ui->addFocusMarker_brightfield->setIcon(QIcon());
		ui->addFocusMarker_brightfield->setText("Ok");
	}
}

void BrillouinAcquisition::drawPositionScannerMarker(POINT2 positionScanner) {
	m_positionScanner = positionScanner;
	// Don't draw if outside of image
	if (m_positionScanner.x < 0 || m_positionScanner.y < 0)	{
		return;
	}

	// Add a marker to the plot to indicate the laser focus
	if (!m_positionScannerMarker) {
		m_positionScannerMarker = m_ODTPlot.plotHandle->addGraph();
		QPen pen;
		pen.setColor(Qt::blue);
		pen.setWidth(2);
		QCPScatterStyle scatterStyle;
		scatterStyle.setShape(QCPScatterStyle::ssCircle);
		scatterStyle.setPen(pen);
		scatterStyle.setSize(8);
		m_positionScannerMarker->setScatterStyle(scatterStyle);
	}
	m_positionScannerMarker->setData(QVector<double>{m_positionScanner.x}, QVector<double>{m_positionScanner.y});
	ui->customplot_brightfield->replot();
}

void BrillouinAcquisition::on_rangeLower_valueChanged(int value) {
	m_BrillouinPlot.cLim.lower = value;
	updatePlot(m_BrillouinPlot);
}

void BrillouinAcquisition::on_rangeUpper_valueChanged(int value) {
	m_BrillouinPlot.cLim.upper = value;
	updatePlot(m_BrillouinPlot);
}

void BrillouinAcquisition::on_rangeLowerODT_valueChanged(int value) {
	m_ODTPlot.cLim.lower = value;
	updatePlot(m_ODTPlot);
}

void BrillouinAcquisition::on_rangeUpperODT_valueChanged(int value) {
	m_ODTPlot.cLim.upper = value;
	updatePlot(m_ODTPlot);
}

void BrillouinAcquisition::updatePlot(const PLOT_SETTINGS& plotSettings) {
	plotSettings.colorMap->setDataRange(plotSettings.cLim);
	plotSettings.plotHandle->replot();
	updateCLimRange(plotSettings.lowerBox, plotSettings.upperBox, plotSettings.cLim);
}

void BrillouinAcquisition::updateCLimRange(QSpinBox *lower, QSpinBox *upper, QCPRange range) {
	const QSignalBlocker blocker1(lower);
	lower->setValue(range.lower);
	lower->setMaximum(range.upper);
	const QSignalBlocker blocker2(upper);
	upper->setValue(range.upper);
	upper->setMinimum(range.lower);
}

void BrillouinAcquisition::xAxisRangeChangedODT(const QCPRange &newRange) {
	m_ODTPlot.plotHandle->xAxis->setRange(newRange.bounded(1, m_cameraOptionsODT.ROIWidthLimits[1]));
}

void BrillouinAcquisition::yAxisRangeChangedODT(const QCPRange &newRange) {
	m_ODTPlot.plotHandle->yAxis->setRange(newRange.bounded(1, m_cameraOptionsODT.ROIHeightLimits[1]));
}

void BrillouinAcquisition::xAxisRangeChanged(QCPRange &newRange) {
	// We need rounded values as this represents pixels
	newRange.lower = floor(newRange.lower);
	newRange.upper = ceil(newRange.upper);

	ui->customplot->xAxis->setRange(newRange.bounded(1, m_cameraOptions.ROIWidthLimits[1]));
	m_deviceSettings.camera.roi.left = newRange.lower;
	m_deviceSettings.camera.roi.width_physical = newRange.upper - newRange.lower + 1;
	settingsCameraUpdate(ROI_SOURCE::PLOT);
}

void BrillouinAcquisition::yAxisRangeChanged(QCPRange &newRange) {
	// We need rounded values as this represents pixels
	newRange.lower = floor(newRange.lower);
	newRange.upper = ceil(newRange.upper);

	ui->customplot->yAxis->setRange(newRange.bounded(1, m_cameraOptions.ROIHeightLimits[1]));
	m_deviceSettings.camera.roi.top = m_cameraOptions.ROIHeightLimits[1] - newRange.upper + 1;
	m_deviceSettings.camera.roi.height_physical = newRange.upper - newRange.lower + 1;
	settingsCameraUpdate(ROI_SOURCE::PLOT);
}

void BrillouinAcquisition::on_ROILeft_valueChanged(int left) {
	m_deviceSettings.camera.roi.left = left;
	settingsCameraUpdate(ROI_SOURCE::BOX);
}

void BrillouinAcquisition::on_ROIWidth_valueChanged(int width) {
	m_deviceSettings.camera.roi.width_physical = width;
	settingsCameraUpdate(ROI_SOURCE::BOX);
}

void BrillouinAcquisition::on_ROITop_valueChanged(int top) {
	m_deviceSettings.camera.roi.top = top;
	settingsCameraUpdate(ROI_SOURCE::BOX);
}

void BrillouinAcquisition::on_ROIHeight_valueChanged(int height) {
	m_deviceSettings.camera.roi.height_physical = height;
	settingsCameraUpdate(ROI_SOURCE::BOX);
}

void BrillouinAcquisition::settingsCameraUpdate(int source) {
	// check that values are valid
	// start must be >= than 1 and <= than (imageSize - minROIsize + 1)
	// ROIsize must be within [minROIsize, maxROIsize]
	// end must be >= than <= than the size of the image

	// check these requirements
	// for x
	std::vector<AT_64> xIn = { m_deviceSettings.camera.roi.left, m_deviceSettings.camera.roi.width_physical };
	std::vector<AT_64> xOut = checkROI(xIn, m_cameraOptions.ROIWidthLimits);
	bool xChanged = (xIn != xOut);
	if (xChanged) {
		m_deviceSettings.camera.roi.left = xOut[0];
		m_deviceSettings.camera.roi.width_physical = xOut[1];
	}

	//for y
	std::vector<AT_64> yIn = { m_deviceSettings.camera.roi.top, m_deviceSettings.camera.roi.height_physical };
	std::vector<AT_64> yOut = checkROI(yIn, m_cameraOptions.ROIHeightLimits);
	bool yChanged = (yIn != yOut);
	if (yChanged) {
		m_deviceSettings.camera.roi.top = yOut[0];
		m_deviceSettings.camera.roi.height_physical = yOut[1];
	}

	// set values of manual input fields
	if (source == ROI_SOURCE::PLOT || xChanged) {
		// dont retrigger a round of checking
		const QSignalBlocker blocker1(ui->ROILeft);
		ui->ROILeft->setValue(m_deviceSettings.camera.roi.left);
		const QSignalBlocker blocker2(ui->ROIWidth);
		ui->ROIWidth->setValue(m_deviceSettings.camera.roi.width_physical);
	}
	if (source == ROI_SOURCE::PLOT || yChanged) {
		// dont retrigger a round of checking
		const QSignalBlocker blocker1(ui->ROITop);
		ui->ROITop->setValue(m_deviceSettings.camera.roi.top);
		const QSignalBlocker blocker2(ui->ROIHeight);
		ui->ROIHeight->setValue(m_deviceSettings.camera.roi.height_physical);
	}
	// set plot range
	//ui->customplot->yAxis->setRange(newRange.bounded(1, m_cameraOptions.ROIHeightLimits[1]));
	if (source == ROI_SOURCE::BOX || xChanged) {
		ui->customplot->xAxis->setRange(QCPRange(m_deviceSettings.camera.roi.left, m_deviceSettings.camera.roi.left + m_deviceSettings.camera.roi.width_physical - 1));
	}
	if (source == ROI_SOURCE::BOX || yChanged) {
		double l = m_cameraOptions.ROIHeightLimits[1] - m_deviceSettings.camera.roi.top - m_deviceSettings.camera.roi.height_physical + 2;
		auto l1 = m_deviceSettings.camera.roi.bottom;
		double h = m_cameraOptions.ROIHeightLimits[1] - m_deviceSettings.camera.roi.top + 1;
		auto h1 = m_deviceSettings.camera.roi.bottom + m_deviceSettings.camera.roi.height_physical - 1;
		ui->customplot->yAxis->setRange(QCPRange(l, h));
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

/*
 * Brillouin camera settings
 */
 // Binning
void BrillouinAcquisition::on_binning_currentIndexChanged(const QString& text) {
	m_Brillouin->settings.camera.roi.binning = text.toStdWString();
	applyCameraSettings();
}
// Readout parameters
void BrillouinAcquisition::on_pixelReadoutRate_currentIndexChanged(const QString& text) {
	m_Brillouin->settings.camera.readout.pixelReadoutRate = text.toStdWString();
	applyCameraSettings();
}

void BrillouinAcquisition::on_preAmpGain_currentIndexChanged(const QString& text) {
	m_Brillouin->settings.camera.readout.preAmpGain = text.toStdWString();
	applyCameraSettings();
}

void BrillouinAcquisition::on_pixelEncoding_currentIndexChanged(const QString& text) {
	m_Brillouin->settings.camera.readout.pixelEncoding = text.toStdWString();
	applyCameraSettings();
}

void BrillouinAcquisition::on_cycleMode_currentIndexChanged(const QString& text) {
	m_Brillouin->settings.camera.readout.cycleMode = text.toStdWString();
	applyCameraSettings();
}

void BrillouinAcquisition::applyCameraSettings() {
	if (!m_andor->m_isPreviewRunning && !m_andor->m_isAcquisitionRunning) {
		m_andor->setSettings(m_Brillouin->settings.camera);
	}
}


void BrillouinAcquisition::updatePlotLimits(const PLOT_SETTINGS& plotSettings,	const CAMERA_OPTIONS& options, const CAMERA_ROI& roi) {
	// set the properties of the colormap to the correct values of the preview buffer
	plotSettings.colorMap->data()->setSize(roi.width_binned, roi.height_binned);
	QCPRange xRange = QCPRange(roi.left, roi.left + roi.width_physical - 1);
	QCPRange yRange = QCPRange(roi.bottom, roi.bottom + roi.height_physical - 1);
	plotSettings.colorMap->data()->setRange(xRange, yRange);

	QCPRange xRangeCurrent = plotSettings.plotHandle->xAxis->range();
	QCPRange yRangeCurrent = plotSettings.plotHandle->yAxis->range();

	QCPRange xRangeNew = QCPRange(
		floor(simplemath::maximum({ xRangeCurrent.lower, xRange.lower })),
		ceil(simplemath::minimum({ xRangeCurrent.upper, xRange.upper }))
	);
	QCPRange yRangeNew = QCPRange(
		floor(simplemath::maximum({ yRangeCurrent.lower, yRange.lower })),
		ceil(simplemath::minimum({ yRangeCurrent.upper, yRange.upper }))
	);

	plotSettings.plotHandle->xAxis->setRange(xRangeNew);
	plotSettings.plotHandle->yAxis->setRange(yRangeNew);
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

void BrillouinAcquisition::showFluorescencePreviewRunning(const FLUORESCENCE_MODE& mode) {
	// reset all preview buttons
	ui->fluoBluePreview->setText("Preview");
	ui->fluoGreenPreview->setText("Preview");
	ui->fluoRedPreview->setText("Preview");
	ui->fluoBrightfieldPreview->setText("Preview");

	// show currently running mode
	switch (mode) {
	case FLUORESCENCE_MODE::BLUE:
		ui->fluoBluePreview->setText("Stop");
		break;
	case FLUORESCENCE_MODE::GREEN:
		ui->fluoGreenPreview->setText("Stop");
		break;
	case FLUORESCENCE_MODE::RED:
		ui->fluoRedPreview->setText("Stop");
		break;
	case FLUORESCENCE_MODE::BRIGHTFIELD:
		ui->fluoBrightfieldPreview->setText("Stop");
		break;
	}
}

void BrillouinAcquisition::startPreview(bool isRunning) {
	// if preview was not running, start it, else leave it running (don't start it twice)
	if (!m_previewRunning && isRunning) {
		updateImageBrillouin();
	}
	m_previewRunning = isRunning;
}

void BrillouinAcquisition::startBrightfieldPreview(bool isRunning) {
	// if preview was not running, start it, else leave it running (don't start it twice)
	if (!m_brightfieldPreviewRunning && isRunning) {
		updateImageODT();
	}
	m_brightfieldPreviewRunning = isRunning;
}

void BrillouinAcquisition::updateImageBrillouin() {
	// Abort updating images when camera is not available
	if (m_andor == nullptr) {
		m_previewRunning = false;
		return;
	}
	updateImage(m_andor->m_previewBuffer, &m_BrillouinPlot);
}

void BrillouinAcquisition::updateImageODT() {
	// Abort updating images when camera is not available
	if (m_brightfieldCamera == nullptr) {
		m_brightfieldPreviewRunning = false;
		return;
	}
	updateImage(m_brightfieldCamera->m_previewBuffer, &m_ODTPlot);
}

template <typename T>
void BrillouinAcquisition::updateImage(PreviewBuffer<T>* previewBuffer, PLOT_SETTINGS *plotSettings) {
	QMetaObject::invokeMethod(
		m_converter,
		[&m_converter = m_converter, previewBuffer, plotSettings]() {
			m_converter->convert(previewBuffer, plotSettings);
		},
		Qt::QueuedConnection
	);
}

void BrillouinAcquisition::plot(PLOT_SETTINGS* plotSettings, long long dim_x, long long dim_y, const std::vector<unsigned char>& unpackedBuffer) {
	plotting(plotSettings, dim_x, dim_y, unpackedBuffer);
}

void BrillouinAcquisition::plot(PLOT_SETTINGS* plotSettings, long long dim_x, long long dim_y, const std::vector<unsigned short>& unpackedBuffer) {
	plotting(plotSettings, dim_x, dim_y, unpackedBuffer);
}

void BrillouinAcquisition::plot(PLOT_SETTINGS* plotSettings, long long dim_x, long long dim_y, const std::vector<double>& unpackedBuffer) {
	plotting(plotSettings, dim_x, dim_y, unpackedBuffer);
}

void BrillouinAcquisition::plot(PLOT_SETTINGS* plotSettings, long long dim_x, long long dim_y, const std::vector<float>& unpackedBuffer) {
	plotting(plotSettings, dim_x, dim_y, unpackedBuffer);
}

void BrillouinAcquisition::plot(PLOT_SETTINGS* plotSettings, long long dim_x, long long dim_y, const std::vector<int>& unpackedBuffer) {
	plotting(plotSettings, dim_x, dim_y, unpackedBuffer);
}

template <typename T>
void BrillouinAcquisition::plotting(PLOT_SETTINGS* plotSettings, long long dim_x, long long dim_y, const std::vector<T>& unpackedBuffer) {
	// images are given row by row, starting at the top left
	int tIndex{ 0 };
	for (gsl::index yIndex{ 0 }; yIndex < dim_y; ++yIndex) {
		for (gsl::index xIndex{ 0 }; xIndex < dim_x; ++xIndex) {
			tIndex = yIndex * dim_x + xIndex;
			plotSettings->colorMap->data()->setCell(xIndex, dim_y - yIndex - 1, unpackedBuffer[tIndex]);
		}
	}
	if (plotSettings->autoscale) {
		plotSettings->colorMap->rescaleDataRange(true);
		plotSettings->cLim = plotSettings->colorMap->dataRange();
		(plotSettings->dataRangeCallback)(plotSettings->cLim);
	}
	plotSettings->plotHandle->replot();
}

void BrillouinAcquisition::on_actionConnect_Camera_triggered() {
	if (m_andor->getConnectionStatus()) {
		QMetaObject::invokeMethod(
			m_andor,
			[&m_andor = m_andor]() {
				m_andor->disconnectDevice();
			},
			Qt::QueuedConnection
		);
	} else {
		QMetaObject::invokeMethod(
			m_andor,
			[&m_andor = m_andor]() {
				m_andor->connectDevice();
			},
			Qt::QueuedConnection
		);
	}
}

void BrillouinAcquisition::cameraConnectionChanged(bool isConnected) {
	if (isConnected) {
		ui->actionConnect_Camera->setText("Disconnect Camera");
		ui->settingsWidget->setTabIcon(0, m_icons.standby);
		ui->actionEnable_Cooling->setEnabled(true);
		ui->camera_playPause->setEnabled(true);
		ui->camera_singleShot->setEnabled(true);
		// switch on cooling automatically
		QMetaObject::invokeMethod(
			m_andor,
			[&m_andor = m_andor]() {
				m_andor->setSensorCooling(true);
			},
			Qt::QueuedConnection
		);
		this->restoreCameraSettings();
	} else {
		ui->actionConnect_Camera->setText("Connect Camera");
		ui->actionEnable_Cooling->setText("Enable Cooling");
		ui->settingsWidget->setTabIcon(0, m_icons.disconnected);
		ui->actionEnable_Cooling->setEnabled(false);
		ui->camera_playPause->setEnabled(false);
		ui->camera_singleShot->setEnabled(false);
	}
}

void BrillouinAcquisition::restoreCameraSettings() {
	QSettings settings(QSettings::IniFormat, QSettings::UserScope,
		"Guck Lab", "Brillouin Acquisition");

	settings.beginGroup("devices-settings");
	auto left = settings.value("brillouin-camera-roi-left", 1).toInt();
	this->on_ROILeft_valueChanged(left);
	auto top = settings.value("brillouin-camera-roi-top", 1).toInt();
	this->on_ROITop_valueChanged(top);
	auto width_phsical = settings.value("brillouin-camera-roi-width-physical").toInt();
	this->on_ROIWidth_valueChanged(width_phsical);
	auto height_phsical = settings.value("brillouin-camera-roi-height-physical").toInt();
	this->on_ROIHeight_valueChanged(height_phsical);
	auto exposureTime = settings.value("brillouin-camera-exposure-time", 0.5).toDouble();
	ui->exposureTime->setValue(exposureTime);
	auto frameCount = settings.value("brillouin-camera-frame-count", 2).toInt();
	ui->frameCount->setValue(frameCount);

	settings.endGroup();
}

void BrillouinAcquisition::showNoCameraFound() {
	QMessageBox::critical(this, "Selected camera not found.", "The selected camera was not found. Switch on the camera and restart the program or select a different camera.");
}

void BrillouinAcquisition::on_actionEnable_Cooling_triggered() {
	if (m_andor->getConnectionStatus()) {
		if (m_andor->getSensorCooling()) {
			QMetaObject::invokeMethod(
				m_andor,
				[&m_andor = m_andor]() {
					m_andor->setSensorCooling(false);
				},
				Qt::QueuedConnection
			);
		} else {
			QMetaObject::invokeMethod(
				m_andor,
				[&m_andor = m_andor]() {
					m_andor->setSensorCooling(true);
				},
				Qt::QueuedConnection
			);
		}
	}
}

void BrillouinAcquisition::cameraCoolingChanged(bool isCooling) {
	if (isCooling) {
		ui->actionEnable_Cooling->setText("Disable Cooling");
		ui->settingsWidget->setTabIcon(0, m_icons.cooling);
	} else {
		ui->actionEnable_Cooling->setText("Enable Cooling");
		ui->settingsWidget->setTabIcon(0, m_icons.standby);
	}
}

void BrillouinAcquisition::on_actionConnect_Stage_triggered() {
	if (m_scanControl->getConnectionStatus()) {
		QMetaObject::invokeMethod(
			m_scanControl,
			[&m_scanControl = m_scanControl]() {
				m_scanControl->disconnectDevice();
			},
			Qt::QueuedConnection
		);
	} else {
		QMetaObject::invokeMethod(
			m_scanControl,
			[&m_scanControl = m_scanControl]() {
				m_scanControl->connectDevice();
			},
			Qt::QueuedConnection
		);
	}
}

void BrillouinAcquisition::microscopeConnectionChanged(bool isConnected) {
	if (isConnected) {
		ui->actionConnect_Stage->setText("Disconnect Microscope");
		ui->settingsWidget->setTabIcon(1, m_icons.ready);
		ui->settingsWidget->setTabIcon(2, m_icons.ready);
	} else {
		ui->actionConnect_Stage->setText("Connect Microscope");
		ui->settingsWidget->setTabIcon(1, m_icons.disconnected);
		ui->settingsWidget->setTabIcon(2, m_icons.disconnected);
	}
}

void BrillouinAcquisition::on_actionConnect_Brightfield_camera_triggered() {
	if (m_brightfieldCamera->getConnectionStatus()) {
		QMetaObject::invokeMethod(
			m_brightfieldCamera,
			[&m_brightfieldCamera = m_brightfieldCamera]() {
				m_brightfieldCamera->disconnectDevice();
			},
			Qt::QueuedConnection
		);
	} else {
		QMetaObject::invokeMethod(
			m_brightfieldCamera,
			[&m_brightfieldCamera = m_brightfieldCamera]() {
				m_brightfieldCamera->connectDevice();
			},
			Qt::QueuedConnection
		);
	}
}

void BrillouinAcquisition::brightfieldCameraConnectionChanged(bool isConnected) {
	if (isConnected) {
		ui->actionConnect_Brightfield_camera->setText("Disconnect Brightfield Camera");
		ui->brightfieldImage->show();
		ui->camera_playPause_brightfield->setEnabled(true);
		ui->settingsWidget->setTabIcon(3, m_icons.ready);
	} else {
		ui->actionConnect_Brightfield_camera->setText("Connect Brightfield Camera");
		ui->brightfieldImage->hide();
		ui->camera_playPause_brightfield->setEnabled(false);
		ui->settingsWidget->setTabIcon(3, m_icons.disconnected);
	}
}

void BrillouinAcquisition::on_camera_playPause_brightfield_clicked() {
	if (!m_brightfieldCamera->m_isPreviewRunning) {
		QMetaObject::invokeMethod(
			m_brightfieldCamera,
			[&m_brightfieldCamera = m_brightfieldCamera]() {
				m_brightfieldCamera->startPreview();
			},
			Qt::QueuedConnection
		);
	} else {
		m_brightfieldCamera->m_stopPreview = true;
		m_Fluorescence->startStopPreview(FLUORESCENCE_MODE::NONE);
	}
}

void BrillouinAcquisition::on_actionSettings_Stage_triggered() {
	m_scanControlDropdown->setCurrentIndex((int)m_scanControllerType);

	auto numberCameras_Brillouin = m_andor->getNumberCameras();
	m_numberCameras_BrillouinDropdown->clear();
	for (gsl::index i{ 0 }; i < numberCameras_Brillouin; i++) {
		m_numberCameras_BrillouinDropdown->insertItem(i, QString::number(i));
	}
	m_numberCameras_BrillouinDropdown->setCurrentIndex((int)m_andor->getCameraNumber());

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
	if (m_cameraBrillouinType != m_cameraBrillouinTypeTemporary ||
		m_cameraBrillouinNumber != m_cameraBrillouinNumberTemporary) {
		m_cameraBrillouinType = m_cameraBrillouinTypeTemporary;
		m_cameraBrillouinNumber = m_cameraBrillouinNumberTemporary;
		initCameraBrillouin();
	}
	m_settingsDialog->hide();
}

void BrillouinAcquisition::cancelSettings() {
	m_scanControllerTypeTemporary = m_scanControllerType;
	m_cameraTypeTemporary = m_cameraType;
	m_cameraBrillouinTypeTemporary = m_cameraBrillouinType;
	m_settingsDialog->hide();
}

void BrillouinAcquisition::initSettingsDialog() {

	if (m_settingsDialog) {
		m_settingsDialog->deleteLater();
		m_settingsDialog = nullptr;
	}
	m_settingsDialog = new QDialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
	m_settingsDialog->setWindowTitle("Settings");
	m_settingsDialog->setWindowModality(Qt::ApplicationModal);

	QVBoxLayout* vLayout = new QVBoxLayout(m_settingsDialog);
	/*
	 * Widget for Brillouin camera selection
	 */
	m_cameraBrillouinTypeTemporary = m_cameraBrillouinType;

	QWidget* cameraBrillouinWidget = new QWidget();
	cameraBrillouinWidget->setMinimumHeight(120);
	cameraBrillouinWidget->setMinimumWidth(250);
	vLayout->addWidget(cameraBrillouinWidget);

	QGroupBox* camBrillouinBox = new QGroupBox(cameraBrillouinWidget);
	camBrillouinBox->setTitle("Brillouin camera");
	camBrillouinBox->setMinimumHeight(50);
	camBrillouinBox->setMinimumWidth(250);

	QVBoxLayout* vCameraLayout = new QVBoxLayout(camBrillouinBox);

	QWidget* cameraTypeWidget = new QWidget();
	vCameraLayout->addWidget(cameraTypeWidget);

	QHBoxLayout* camBrillouinLayout = new QHBoxLayout(cameraTypeWidget);

	QLabel* camera_BrillouinLabel = new QLabel("Camera type");
	camBrillouinLayout->addWidget(camera_BrillouinLabel);

	m_camera_BrillouinDropdown = new QComboBox();
	camBrillouinLayout->addWidget(m_camera_BrillouinDropdown);
	gsl::index i{ 0 };
	for (auto type : CAMERA_BRILLOUIN_DEVICE_NAMES) {
		m_camera_BrillouinDropdown->insertItem(i, QString::fromStdString(type));
		i++;
	}
	m_camera_BrillouinDropdown->setCurrentIndex((int)m_cameraBrillouinType);

	if (m_andor) {
		QWidget* cameraNumberWidget = new QWidget();
		vCameraLayout->addWidget(cameraNumberWidget);
		QHBoxLayout* numberCameras_BrillouinLayout = new QHBoxLayout(cameraNumberWidget);

		QLabel* numberCameras_BrillouinLabel = new QLabel("Camera number");
		numberCameras_BrillouinLayout->addWidget(numberCameras_BrillouinLabel);

		m_numberCameras_BrillouinDropdown = new QComboBox();
		numberCameras_BrillouinLayout->addWidget(m_numberCameras_BrillouinDropdown);
		auto numberCameras_Brillouin = m_andor->getNumberCameras();
		for (gsl::index i{ 0 }; i < numberCameras_Brillouin; i++) {
			m_numberCameras_BrillouinDropdown->insertItem(i, QString::number(i));
		}
		m_numberCameras_BrillouinDropdown->setCurrentIndex((int)m_andor->getCameraNumber());
	}

	static QMetaObject::Connection connection = QWidget::connect<void(QComboBox::*)(int)>(
		m_camera_BrillouinDropdown,
		&QComboBox::currentIndexChanged,
		this,
		[this](int index) { selectCameraBrillouinDevice(index); }
	);

	connection = QWidget::connect<void(QComboBox::*)(int)>(
		m_numberCameras_BrillouinDropdown,
		&QComboBox::currentIndexChanged,
		this,
		[this](int index) { selectCameraBrillouinNumber(index); }
	);

	/*
	 * Widget for scan controller selection
	 */
	m_scanControllerTypeTemporary = m_scanControllerType;

	QWidget *daqWidget = new QWidget();
	daqWidget->setMinimumHeight(60);
	daqWidget->setMinimumWidth(250);
	QGroupBox *box = new QGroupBox(daqWidget);
	box->setTitle("Scanning device");
	box->setMinimumHeight(50);
	box->setMinimumWidth(250);

	vLayout->addWidget(daqWidget);

	QHBoxLayout *layout = new QHBoxLayout(box);

	QLabel *label = new QLabel("Currently selected device");
	layout->addWidget(label);

	m_scanControlDropdown = new QComboBox();
	layout->addWidget(m_scanControlDropdown);
	i = 0;
	for (auto type : ScanControl::SCAN_DEVICE_NAMES) {
		m_scanControlDropdown->insertItem(i, QString::fromStdString(type));
		i++;
	}
	m_scanControlDropdown->setCurrentIndex((int)m_scanControllerType);

	connection = QWidget::connect<void(QComboBox::*)(int)>(
		m_scanControlDropdown,
		&QComboBox::currentIndexChanged,
		this,
		[this](int index) { selectScanningDevice(index); }
	);

	/*
	 * Widget for ODT/Fluorescence camera selection
	 */
	m_cameraTypeTemporary = m_cameraType;

	QWidget *cameraWidget = new QWidget();
	cameraWidget->setMinimumHeight(60);
	cameraWidget->setMinimumWidth(250);
	QGroupBox *camBox = new QGroupBox(cameraWidget);
	camBox->setTitle("ODT/Fluorescence camera");
	camBox->setMinimumHeight(50);
	camBox->setMinimumWidth(250);

	vLayout->addWidget(cameraWidget);

	QHBoxLayout *camLayout = new QHBoxLayout(camBox);

	QLabel *camLabel = new QLabel("Camera type");
	camLayout->addWidget(camLabel);

	m_cameraDropdown = new QComboBox();
	camLayout->addWidget(m_cameraDropdown);
	i = 0;
	for (auto type : CAMERA_DEVICE_NAMES) {
		m_cameraDropdown->insertItem(i, QString::fromStdString(type));
		i++;
	}
	m_cameraDropdown->setCurrentIndex((int)m_cameraType);

	connection = QWidget::connect<void(QComboBox::*)(int)>(
		m_cameraDropdown,
		&QComboBox::currentIndexChanged,
		this,
		[this](int index) { selectCameraDevice(index); }
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
	okButton->setMinimumWidth(60);
	okButton->setMaximumWidth(60);
	buttonLayout->addWidget(okButton);
	buttonLayout->setAlignment(okButton, Qt::AlignRight);

	connection = QWidget::connect(
		okButton,
		&QPushButton::clicked,
		this,
		[this]() { saveSettings(); }
	);

	QPushButton *cancelButton = new QPushButton();
	cancelButton->setText(tr("Cancel"));
	cancelButton->setMinimumWidth(60);
	cancelButton->setMaximumWidth(60);
	buttonLayout->addWidget(cancelButton);

	connection = QWidget::connect(
		cancelButton,
		&QPushButton::clicked,
		this,
		[this]() { cancelSettings(); }
	);

	m_settingsDialog->layout()->setSizeConstraint(QLayout::SetFixedSize);
}

void BrillouinAcquisition::selectScanningDevice(int index) {
	m_scanControllerTypeTemporary = (ScanControl::SCAN_DEVICE)index;
}

void BrillouinAcquisition::selectCameraDevice(int index) {
	m_cameraTypeTemporary = (CAMERA_DEVICE)index;
}

void BrillouinAcquisition::selectCameraBrillouinDevice(int index) {
	m_cameraBrillouinTypeTemporary = (CAMERA_BRILLOUIN_DEVICE)index;
}

void BrillouinAcquisition::selectCameraBrillouinNumber(int index) {
	m_cameraBrillouinNumberTemporary = index;
}

void BrillouinAcquisition::on_action_Voltage_calibration_acquire_triggered() {
	QMetaObject::invokeMethod(
		m_voltageCalibration,
		[&m_voltageCalibration = m_voltageCalibration]() {
			m_voltageCalibration->startRepetitions();
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::on_action_Voltage_calibration_load_triggered() {
	m_voltageCalibrationFilePath = QFileDialog::getOpenFileName(this, tr("Select Voltage-Position map"),
		QString::fromStdString(m_voltageCalibrationFilePath), tr("Calibration map (*.h5)")).toStdString();
	m_voltageCalibration->load(m_voltageCalibrationFilePath);
}

void BrillouinAcquisition::on_action_Scale_calibration_acquire_triggered() {
	if (!m_scaleCalibrationDialog) {
		m_scaleCalibrationDialog = new QDialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
	}
	m_scaleCalibrationDialogUi.setupUi(m_scaleCalibrationDialog);

	m_scaleCalibrationDialog->setWindowTitle("Scale calibration");
	m_scaleCalibrationDialog->setWindowModality(Qt::ApplicationModal);

	if (!m_brightfieldCamera) {
		m_scaleCalibrationDialogUi.button_acquire->setDisabled(true);
	} else {
		m_scaleCalibrationDialogUi.button_acquire->setDisabled(false);
	}

	// Connect close signal
	auto connection = QWidget::connect(
		m_scaleCalibrationDialog,
		&QDialog::rejected,
		this,
		[this]() { closeScaleCalibrationDialog(); }
	);

	// Connect push buttons
	connection = QWidget::connect(
		m_scaleCalibrationDialogUi.button_cancel,
		&QPushButton::clicked,
		this,
		[this]() { closeScaleCalibrationDialog(); }
	);
	connection = QWidget::connect(
		m_scaleCalibrationDialogUi.button_apply,
		&QPushButton::clicked,
		this,
		[this]() { scaleCalibrationButtonApply_clicked(); }
	);
	connection = QWidget::connect(
		m_scaleCalibrationDialogUi.button_acquire,
		&QPushButton::clicked,
		this,
		[this]() { scaleCalibrationButtonAcquire_clicked(); }
	);

	// Connect translation distance boxes
	connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
		m_scaleCalibrationDialogUi.dx,
		&QDoubleSpinBox::valueChanged,
		this,
		[this](double dx) { setTranslationDistanceX(dx); }
	);
	connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
		m_scaleCalibrationDialogUi.dy,
		&QDoubleSpinBox::valueChanged,
		this,
		[this](double dy) { setTranslationDistanceY(dy); }
	);

	// Connect scale calibration boxes
	connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
		m_scaleCalibrationDialogUi.micrometerToPixX_x,
		&QDoubleSpinBox::valueChanged,
		this,
		[this](double value) { setMicrometerToPixX_x(value); }
	);
	connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
		m_scaleCalibrationDialogUi.micrometerToPixX_y,
		&QDoubleSpinBox::valueChanged,
		this,
		[this](double value) { setMicrometerToPixX_y(value); }
	);
	connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
		m_scaleCalibrationDialogUi.micrometerToPixY_x,
		&QDoubleSpinBox::valueChanged,
		this,
		[this](double value) { setMicrometerToPixY_x(value); }
	);
	connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
		m_scaleCalibrationDialogUi.micrometerToPixY_y,
		&QDoubleSpinBox::valueChanged,
		this,
		[this](double value) { setMicrometerToPixY_y(value); }
	);

	connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
		m_scaleCalibrationDialogUi.pixToMicrometerX_x,
		&QDoubleSpinBox::valueChanged,
		this,
		[this](double value) { setPixToMicrometerX_x(value); }
	);
	connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
		m_scaleCalibrationDialogUi.pixToMicrometerX_y,
		&QDoubleSpinBox::valueChanged,
		this,
		[this](double value) { setPixToMicrometerX_y(value); }
	);
	connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
		m_scaleCalibrationDialogUi.pixToMicrometerY_x,
		&QDoubleSpinBox::valueChanged,
		this,
		[this](double value) { setPixToMicrometerY_x(value); }
	);
	connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
		m_scaleCalibrationDialogUi.pixToMicrometerY_y,
		&QDoubleSpinBox::valueChanged,
		this,
		[this](double value) { setPixToMicrometerY_y(value); }
	);

	// Initialize the scaleCalibration
	m_scaleCalibration->initialize();

	m_scaleCalibrationDialog->show();
}

void BrillouinAcquisition::updateScaleCalibrationTranslationValue(POINT2 translation) {
	// Initialize all values
	m_scaleCalibrationDialogUi.dx->setValue(translation.x);
	m_scaleCalibrationDialogUi.dy->setValue(translation.y);
}

void BrillouinAcquisition::updateScaleCalibrationData(ScaleCalibrationData scaleCalibration) {
	// We have to block the signals so that programmatically setting new values
	// doesn't trigger a new round of calculations
	const QSignalBlocker blocker1(m_scaleCalibrationDialogUi.micrometerToPixX_x);
	const QSignalBlocker blocker2(m_scaleCalibrationDialogUi.micrometerToPixX_y);
	const QSignalBlocker blocker3(m_scaleCalibrationDialogUi.micrometerToPixY_x);
	const QSignalBlocker blocker4(m_scaleCalibrationDialogUi.micrometerToPixY_y);

	const QSignalBlocker blocker5(m_scaleCalibrationDialogUi.pixToMicrometerX_x);
	const QSignalBlocker blocker6(m_scaleCalibrationDialogUi.pixToMicrometerX_y);
	const QSignalBlocker blocker7(m_scaleCalibrationDialogUi.pixToMicrometerY_x);
	const QSignalBlocker blocker8(m_scaleCalibrationDialogUi.pixToMicrometerY_y);

	m_scaleCalibrationDialogUi.micrometerToPixX_x->setValue(scaleCalibration.micrometerToPixX.x);
	m_scaleCalibrationDialogUi.micrometerToPixX_y->setValue(scaleCalibration.micrometerToPixX.y);
	m_scaleCalibrationDialogUi.micrometerToPixY_x->setValue(scaleCalibration.micrometerToPixY.x);
	m_scaleCalibrationDialogUi.micrometerToPixY_y->setValue(scaleCalibration.micrometerToPixY.y);

	m_scaleCalibrationDialogUi.pixToMicrometerX_x->setValue(scaleCalibration.pixToMicrometerX.x);
	m_scaleCalibrationDialogUi.pixToMicrometerX_y->setValue(scaleCalibration.pixToMicrometerX.y);
	m_scaleCalibrationDialogUi.pixToMicrometerY_x->setValue(scaleCalibration.pixToMicrometerY.x);
	m_scaleCalibrationDialogUi.pixToMicrometerY_y->setValue(scaleCalibration.pixToMicrometerY.y);
}

void BrillouinAcquisition::closeScaleCalibrationDialog() {
	if (m_scaleCalibrationDialog) {
		m_scaleCalibrationDialog->hide();
	}
}

void BrillouinAcquisition::updateScaleCalibrationAcquisitionProgress(double progress) {
	m_scaleCalibrationDialogUi.progress->setValue(progress);
}

void BrillouinAcquisition::showScaleCalibrationStatus(std::string title, std::string message) {
	auto msgBox = QMessageBox(
		QMessageBox::Icon::Warning,
		QString::fromStdString(title),
		QString::fromStdString(message),
		QMessageBox::StandardButton::Close,
		this,
		Qt::WindowTitleHint | Qt::WindowCloseButtonHint
	);
	msgBox.exec();
}

void BrillouinAcquisition::scaleCalibrationButtonApply_clicked() {
	QMetaObject::invokeMethod(
		m_scaleCalibration,
		[&m_scaleCalibration = m_scaleCalibration]() {
			m_scaleCalibration->apply();
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::scaleCalibrationButtonAcquire_clicked() {
	QMetaObject::invokeMethod(
		m_scaleCalibration,
		[&m_scaleCalibration = m_scaleCalibration]() {
			m_scaleCalibration->startRepetitions();
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::setTranslationDistanceX(double dx) {
	m_scaleCalibration->setTranslationDistanceX(dx);
}

void BrillouinAcquisition::setTranslationDistanceY(double dy) {
	m_scaleCalibration->setTranslationDistanceY(dy);
}

void BrillouinAcquisition::setMicrometerToPixX_x(double value) {
	m_scaleCalibration->setMicrometerToPixX_x(value);
}

void BrillouinAcquisition::setMicrometerToPixX_y(double value) {
	m_scaleCalibration->setMicrometerToPixX_y(value);
}

void BrillouinAcquisition::setMicrometerToPixY_x(double value) {
	m_scaleCalibration->setMicrometerToPixY_x(value);
}

void BrillouinAcquisition::setMicrometerToPixY_y(double value) {
	m_scaleCalibration->setMicrometerToPixY_y(value);
}

void BrillouinAcquisition::setPixToMicrometerX_x(double value) {
	m_scaleCalibration->setPixToMicrometerX_x(value);
}

void BrillouinAcquisition::setPixToMicrometerX_y(double value) {
	m_scaleCalibration->setPixToMicrometerX_y(value);
}

void BrillouinAcquisition::setPixToMicrometerY_x(double value) {
	m_scaleCalibration->setPixToMicrometerY_x(value);
}

void BrillouinAcquisition::setPixToMicrometerY_y(double value) {
	m_scaleCalibration->setPixToMicrometerY_y(value);
}

void BrillouinAcquisition::on_action_Scale_calibration_load_triggered() {
	m_scaleCalibrationFilePath = QFileDialog::getOpenFileName(this, tr("Select scale calibration"),
		QString::fromStdString(m_scaleCalibrationFilePath), tr("Scale calibration (*.h5)")).toStdString();
	loadScaleCalibrationFile();
	writeSettings();
}

void BrillouinAcquisition::loadScaleCalibrationFile() {
	QMetaObject::invokeMethod(
		m_scaleCalibration,
		[&m_scaleCalibration = m_scaleCalibration, &m_scaleCalibrationFilePath = m_scaleCalibrationFilePath]() {
			m_scaleCalibration->load(m_scaleCalibrationFilePath);
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::initBeampathButtons() {
	if (m_scanControl == nullptr) {
		return;
	}

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
	int maxWidgetsPerRow{ 4 };
	QMetaObject::Connection connection;
	QVBoxLayout *verticalLayout = new QVBoxLayout;
	verticalLayout->setAlignment(Qt::AlignTop);
	std::string buttonLabel;
	presetButtons.clear();
	auto presets = m_scanControl->m_presets;
	QWidget* presetWidget = new QWidget();
	int rows{ 0 };
	if (presets.size() > 0) {
		// Create preset Widget and set vertical layout
		verticalLayout->addWidget(presetWidget);
		QVBoxLayout* presetWidgetLayout = new QVBoxLayout(presetWidget);
		presetWidgetLayout->setMargin(0);

		// Horizontal layout for preset label
		QHBoxLayout * presetLabelLayout = new QHBoxLayout();
		QLabel *presetLabel = new QLabel("Presets:");
		presetLabelLayout->addWidget(presetLabel);
		presetWidgetLayout->addLayout(presetLabelLayout);

		// Grid layout for preset buttons
		QGridLayout *layout = new QGridLayout();
		layout->setAlignment(Qt::AlignLeft);
		for (gsl::index ii = 0; ii < presets.size(); ii++) {
			QPushButton *button = new QPushButton(presets[ii].name.c_str());
			button->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
			button->setMinimumWidth(24);
			button->setMaximumWidth(64);
			button->setMinimumHeight(18);
			layout->addWidget(button, 1 + floor(ii/maxWidgetsPerRow), ii%maxWidgetsPerRow, Qt::AlignLeft);

			connection = QObject::connect(button, &QPushButton::clicked, [=] {
				setPreset(presets[ii].index);
			});
			presetButtons.push_back(button);
		}
		int presetRows = (1 + ceil(presets.size() / (double)maxWidgetsPerRow));
		rows += presetRows;
		int height = presetRows * 20;
		presetWidget->setMinimumHeight(height);
		presetWidgetLayout->addLayout(layout);
	}

	QWidget *elementButtonWidget = new QWidget();
	elementButtonWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	verticalLayout->addWidget(elementButtonWidget);
	QVBoxLayout* elementButtonWidgetLayout = new QVBoxLayout(elementButtonWidget);
	elementButtonWidgetLayout->setMargin(0);

	elementButtons.clear();
	elementIntBox.clear();
	elementDoubleBox.clear();
	elementSlider.clear();
	elementSliderInput.clear();
	auto elements = m_scanControl->m_deviceElements;
	rows += elements.size();
	elementButtonWidget->setMinimumHeight(elements.size() * 20);
	for (gsl::index ii{ 0 }; ii < elements.size(); ii++) {
		DeviceElement element = elements[ii];
		QHBoxLayout *layout = new QHBoxLayout();

		layout->setAlignment(Qt::AlignLeft);
		QLabel *groupLabel = new QLabel(element.name.c_str());
		groupLabel->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
		groupLabel->setMinimumWidth(40);
		groupLabel->setMaximumWidth(40);
		layout->addWidget(groupLabel);
		if (element.inputType == DEVICE_INPUT_TYPE::PUSHBUTTON) {
			std::vector<QPushButton*> buttons;
			for (gsl::index jj = 0; jj < element.maxOptions; jj++) {
				QPushButton *button = new QPushButton(element.optionNames[jj].c_str());
				button->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
				button->setMinimumWidth(16);
				button->setMaximumWidth(48);
				layout->addWidget(button);

				connection = QObject::connect(button, &QPushButton::clicked, [=] {
					setElement(elements[ii], (double)(jj + 1));
				});
				buttons.push_back(button);
			}
			elementButtons.push_back(buttons);
		} else if (element.inputType == DEVICE_INPUT_TYPE::INTBOX) {
			QSpinBox *intBox = new QSpinBox();
			intBox->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
			intBox->setMinimum(-10000);
			intBox->setMaximum(10000);
			intBox->setMinimumWidth(48);
			intBox->setMaximumWidth(96);
			layout->addWidget(intBox);

			connection = QObject::connect<void(QSpinBox::*)(const int)>(
				intBox,
				&QSpinBox::valueChanged,
				[=](int value) {setElement(elements[ii], (double)value);}
			);
			elementIntBox.push_back(intBox);
		} else if (element.inputType == DEVICE_INPUT_TYPE::DOUBLEBOX) {
			QDoubleSpinBox *doubleBox = new QDoubleSpinBox();
			doubleBox->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
			doubleBox->setMinimum(-10000.0);
			doubleBox->setMaximum(10000.0);
			doubleBox->setMinimumWidth(48);
			doubleBox->setMaximumWidth(96);
			layout->addWidget(doubleBox);

			connection = QObject::connect<void(QDoubleSpinBox::*)(const double)>(
				doubleBox,
				&QDoubleSpinBox::valueChanged,
				[=](double value) {setElement(elements[ii], value); }
			);
			elementDoubleBox.push_back(doubleBox);
		} else if (element.inputType == DEVICE_INPUT_TYPE::SLIDER) {
			QSlider* slider = new QSlider();
			slider->setTracking(false);
			slider->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
			slider->setMinimum(0);
			slider->setMaximum(100);
			slider->setSingleStep(1.0);
			slider->setPageStep(5.0);
			slider->setOrientation(Qt::Horizontal);
			slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
			layout->addWidget(slider);

			QSpinBox* intBox = new QSpinBox();
			intBox->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
			intBox->setMinimumWidth(48);
			intBox->setMaximumWidth(96);
			intBox->setMinimum(0);
			intBox->setMaximum(100);
			intBox->setSingleStep(1.0);
			layout->addWidget(intBox);

			connection = QObject::connect<void(QSlider::*)(const int)>(
				slider,
				&QSlider::valueChanged,
				[=](int value) {
					setElement(elements[ii], value);
					const QSignalBlocker blocker(intBox);
					intBox->setValue(value);
				}
			);

			connection = QObject::connect<void(QSpinBox::*)(const int)>(
				intBox,
				&QSpinBox::valueChanged,
				[=](int value) {
					setElement(elements[ii], (double)value);
					const QSignalBlocker blocker(slider);
					slider->setValue(value);
				}
			);

			elementSlider.push_back(slider);
			elementSliderInput.push_back(intBox);
		}
		elementButtonWidgetLayout->addLayout(layout);
	}

	ui->beamPathBox->setLayout(verticalLayout);
	ui->beamPathBox->show();
	ui->beamPathBox->setMinimumHeight(rows * 20 + 15);
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
			break;
		case ScanControl::SCAN_DEVICE::NIDAQ:
			m_scanControl = new NIDAQ();
			break;
		case ScanControl::SCAN_DEVICE::ZEISSMTB:
			m_scanControl = new ZeissMTB();
			break;
		case ScanControl::SCAN_DEVICE::ZEISSMTBERLANGEN:
			m_scanControl = new ZeissMTB_Erlangen();
			break;
		case ScanControl::SCAN_DEVICE::ZEISSMTBERLANGEN2:
			m_scanControl = new ZeissMTB_Erlangen2();
			break;
		default:
			m_scanControl = new ZeissECU();
			break;
	}

	// init or de-init ODT
	initODT();
	initVoltageCalibration();
	initScaleCalibration();

	initializeLaserPositionLocation();

	// Update positions preview
	AOI_changed(m_positionsMicrometer);

	// reestablish m_scanControl connections
	static QMetaObject::Connection connection;
	connection = QWidget::connect(
		m_scanControl,
		&ScanControl::connectedDevice,
		this,
		[this](bool isConnected) { microscopeConnectionChanged(isConnected); }
	);

	// slot to update microscope element button background color
	connection = QWidget::connect(
		m_scanControl,
		&ScanControl::elementPositionsChanged,
		this,
		[this](std::vector<double> positions) { microscopeElementPositionsChanged(positions); }
	);
	connection = QWidget::connect(
		m_scanControl,
		&ScanControl::elementPositionChanged,
		this,
		[this](DeviceElement element, double position) { microscopeElementPositionChanged(element, position); }
	);
	connection = QWidget::connect(
		m_scanControl,
		&ScanControl::currentPosition,
		this,
		[this](POINT3 position) { showPosition(position); }
	);
	connection = QWidget::connect(
		&buttonDelegate,
		&ButtonDelegate::deletePosition,
		this->m_scanControl,
		[this](int index) { this->m_scanControl->deleteSavedPosition(index); }
	);
	connection = QWidget::connect(
		&buttonDelegate,
		&ButtonDelegate::moveToPosition,
		this->m_scanControl,
		[this](int index) { this->m_scanControl->moveToSavedPosition(index); }
	);
	connection = QWidget::connect(
		m_scanControl,
		&ScanControl::savedPositionsChanged,
		this->tableModel,
		[this](std::vector<POINT3> storage) { this->tableModel->setStorage(storage); }
	);
	connection = QWidget::connect(
		m_scanControl,
		&ScanControl::homePositionBoundsChanged,
		this,
		[this](BOUNDS bounds) { setHomePositionBounds(bounds); }
	);
	connection = QWidget::connect(
		m_scanControl,
		&ScanControl::currentPositionBoundsChanged,
		this,
		[this](BOUNDS bounds) { setCurrentPositionBounds(bounds); }
	);
	connection = QWidget::connect(
		m_scanControl,
		&ScanControl::s_scaleCalibrationChanged,
		this,
		[this](std::vector<POINT2> positions) { on_scaleCalibrationChanged(positions); }
	);
	connection = QWidget::connect(
		m_scanControl,
		&ScanControl::s_positionScannerChanged,
		this,
		[this](POINT2 position) { drawPositionScannerMarker(position); }
	);
	tableModel->setStorage(m_scanControl->getSavedPositionsNormalized());

	m_acquisitionThread.startWorker(m_scanControl);

	QMetaObject::invokeMethod(
		m_scanControl,
		[&m_scanControl = m_scanControl]() {
			m_scanControl->connectDevice();
		},
		Qt::AutoConnection
	);

	loadScaleCalibrationFile();

	m_scanControl->locatePositionScanner(m_positionScanner);
}

void BrillouinAcquisition::initODT() {
	if (m_ODT) {
		m_ODT->deleteLater();
		m_ODT = nullptr;
	}
	if (!m_scanControl->supportsCapability(Capabilities::ODT)) {
		int tabIndexODT = ui->acquisitionModeTabs->indexOf(ui->ODT);
		if (tabIndexODT > -1) {
			ui->acquisitionModeTabs->removeTab(tabIndexODT);
		}
	} else {
		m_ODT = new ODT(nullptr, m_acquisition, m_brightfieldCamera, (ODTControl*&)m_scanControl);
		ui->acquisitionModeTabs->insertTab(1, ui->ODT, "ODT");

		static QMetaObject::Connection connection;
		connection = QWidget::connect(
			m_ODT,
			&ODT::s_acqSettingsChanged,
			this,
			[this](ODT_SETTINGS settings) { plotODTVoltages(settings, ODT_MODE::ACQ); }
		);

		connection = QWidget::connect(
			m_ODT,
			&ODT::s_algnSettingsChanged,
			this,
			[this](ODT_SETTINGS settings) { plotODTVoltages(settings, ODT_MODE::ALGN); }
		);

		connection = QWidget::connect(
			m_ODT,
			&ODT::s_mirrorVoltageChanged,
			this,
			[this](VOLTAGE2 voltage, ODT_MODE mode) { plotODTVoltage(voltage, mode); }
		);

		connection = QWidget::connect(
			m_ODT,
			&ODT::s_cameraSettingsChanged,
			this,
			[this](CAMERA_SETTINGS settings) { updateODTCameraSettings(settings); }
		);

		// slot to show current acquisition state
		connection = QWidget::connect(
			m_ODT,
			&ODT::s_acquisitionStatus,
			this,
			[this](ACQUISITION_STATUS state) { showODTStatus(state); }
		);

		// slot to show current repetition progress
		connection = QWidget::connect(
			m_ODT,
			&ODT::s_repetitionProgress,
			this,
			[this](double progress, int seconds) { showODTProgress(progress, seconds); }
		);

		// start ODT thread
		m_acquisitionThread.startWorker(m_ODT);
		m_ODT->initialize();
	}
}

void BrillouinAcquisition::initVoltageCalibration() {
	if (m_voltageCalibration) {
		m_voltageCalibration->deleteLater();
		m_voltageCalibration = nullptr;
	}
	if (!m_scanControl->supportsCapability(Capabilities::VoltageCalibration)) {
		ui->menu_Voltage_calibration->menuAction()->setVisible(false);
	} else {
		m_voltageCalibration = new VoltageCalibration(nullptr, m_acquisition, m_brightfieldCamera, (ODTControl*&)m_scanControl);
		ui->menu_Voltage_calibration->menuAction()->setVisible(true);

		static QMetaObject::Connection connection;
		connection = QWidget::connect(
			m_voltageCalibration,
			&VoltageCalibration::s_cameraSettingsChanged,
			this,
			[this](CAMERA_SETTINGS settings) { updateODTCameraSettings(settings); }
		);
		connection = QWidget::connect(
			m_voltageCalibration,
			&VoltageCalibration::s_voltageCalibrationStatus,
			this,
			[this](std::string title, std::string message) { showScaleCalibrationStatus(title, message); }
		);

		// start Calibration thread
		m_acquisitionThread.startWorker(m_voltageCalibration);
	}
}

void BrillouinAcquisition::initScaleCalibration() {
	// If the scanControl does not support ScaleCalibration, we hide the "Acquire" entry.
	if (!m_scanControl->supportsCapability(Capabilities::ScaleCalibration)) {
		ui->action_Scale_calibration_acquire->setVisible(false);
	} else {
		ui->action_Scale_calibration_acquire->setVisible(true);
	}

	// Initialize scaleCalibration if it is not running already.
	if (!m_scaleCalibration) {
		m_scaleCalibration = new ScaleCalibration(nullptr, m_acquisition, m_brightfieldCamera, m_scanControl);
		// start Calibration thread
		m_acquisitionThread.startWorker(m_scaleCalibration);

		// Connect signals/slots for updating values
		auto connection = QWidget::connect(
			m_scaleCalibration,
			&ScaleCalibration::s_Ds_changed,
			this,
			[this](POINT2 translation) { updateScaleCalibrationTranslationValue(translation); }
		);
		connection = QWidget::connect(
			m_scaleCalibration,
			&ScaleCalibration::s_scaleCalibrationChanged,
			this,
			[this](ScaleCalibrationData scaleCalibration) { updateScaleCalibrationData(scaleCalibration); }
		);
		connection = QWidget::connect(
			m_scaleCalibration,
			&ScaleCalibration::s_scaleCalibrationAcquisitionProgress,
			this,
			[this](double progress) { updateScaleCalibrationAcquisitionProgress(progress); }
		);
		connection = QWidget::connect(
			m_scaleCalibration,
			&ScaleCalibration::s_scaleCalibrationStatus,
			this,
			[this](std::string title, std::string message) { showScaleCalibrationStatus(title, message); }
		);
		connection = QWidget::connect(
			m_scaleCalibration,
			&ScaleCalibration::s_closeScaleCalibrationDialog,
			this,
			[this]() { closeScaleCalibrationDialog(); }
		);
	}
}

void BrillouinAcquisition::initFluorescence() {
	if (m_Fluorescence) {
		m_Fluorescence->deleteLater();
		m_Fluorescence = nullptr;
	}
	if (!m_hasFluorescence) {
		int tabIndexFluorescence = ui->acquisitionModeTabs->indexOf(ui->Fluorescence);
		if (tabIndexFluorescence > -1) {
			ui->acquisitionModeTabs->removeTab(tabIndexFluorescence);
		}
	} else {
		m_Fluorescence = new Fluorescence(nullptr, m_acquisition, m_brightfieldCamera, m_scanControl);
		ui->acquisitionModeTabs->insertTab(2, ui->Fluorescence, "Fluorescence");

		static QMetaObject::Connection connection;
		connection = QWidget::connect(
			m_Fluorescence,
			&Fluorescence::s_acqSettingsChanged,
			this,
			[this](FLUORESCENCE_SETTINGS settings) { updateFluorescenceSettings(settings); }
		);

		// slot to show current acquisition state of Fluorescence mode
		connection = QWidget::connect(
			m_Fluorescence,
			&Fluorescence::s_acquisitionStatus,
			this,
			[this](ACQUISITION_STATUS state) { showFluorescenceStatus(state); }
		);

		// slot to show current repetition progress
		connection = QWidget::connect(
			m_Fluorescence,
			&Fluorescence::s_repetitionProgress,
			this,
			[this](double progress, int seconds) { showFluorescenceProgress(progress, seconds); }
		);

		// slot to show current repetition progress
		connection = QWidget::connect(
			m_Fluorescence,
			&Fluorescence::s_previewRunning,
			this,
			[this](FLUORESCENCE_MODE mode) { showFluorescencePreviewRunning(mode); }
		);

		// start Fluorescence thread
		m_acquisitionThread.startWorker(m_Fluorescence);
		m_Fluorescence->initialize();

	}
}

void BrillouinAcquisition::initCameraBrillouin() {
	// deinitialize camera if necessary
	if (m_andor) {
		m_andor->deleteLater();
		m_andor = nullptr;
	}

	// initialize correct camera type
	switch (m_cameraBrillouinType) {
		case CAMERA_BRILLOUIN_DEVICE::ANDOR:
			m_andor = new Andor();
			break;
		case CAMERA_BRILLOUIN_DEVICE::PVCAM:
			m_andor = new PVCamera();
			break;
#ifdef _DEBUG
		case CAMERA_BRILLOUIN_DEVICE::MOCK:
			m_andor = new MockCamera();
			break;
#endif
		default:
			m_andor = new Andor();
			break;
	}
	// Select which camera to connect to
	m_andor->setCameraNumber(m_cameraBrillouinNumber);

	// slot camera connection
	static QMetaObject::Connection connection;
	connection = QWidget::connect(
		m_andor,
		&Camera::s_previewBufferSettingsChanged,
		this,
		[this] { updatePlotLimits(m_BrillouinPlot, m_cameraOptions, m_andor->m_previewBuffer->m_bufferSettings.roi); }
	);

	connection = QWidget::connect(
		m_andor,
		&Camera::connectedDevice,
		this,
		[this](bool isConnected) { cameraConnectionChanged(isConnected); }
	);

	connection = QWidget::connect(
		m_andor,
		&Camera::noCameraFound,
		this,
		[this] { showNoCameraFound(); }
	);

	connection = QWidget::connect(
		m_andor,
		&Camera::cameraCoolingChanged,
		this,
		[this](bool isCooling) { cameraCoolingChanged(isCooling); }
	);

	connection = QWidget::connect(
		m_andor,
		&Camera::s_imageReady,
		this,
		[this] { updateImageBrillouin(); }
	);

	connection = QWidget::connect(
		m_andor,
		&Camera::s_previewRunning,
		this,
		[this](bool isRunning) { showPreviewRunning(isRunning); }
	);

	connection = QWidget::connect(
		m_andor,
		&Camera::s_acquisitionRunning,
		this,
		[this](bool isRunning) { startPreview(isRunning); }
	);

	connection = QWidget::connect(
		m_andor,
		&Camera::optionsChanged,
		this,
		[this](CAMERA_OPTIONS options) { cameraOptionsChanged(options); }
	);

	connection = QWidget::connect(
		m_andor,
		&Camera::settingsChanged,
		this,
		[this](CAMERA_SETTINGS settings) { cameraSettingsChanged(settings); }
	);

	connection = QWidget::connect(
		m_andor,
		&Camera::s_sensorTemperatureChanged,
		this,
		[this](SensorTemperature sensorTemperature) { sensorTemperatureChanged(sensorTemperature); }
	);

	// start andor thread
	m_andorThread.startWorker(m_andor);

	QMetaObject::invokeMethod(
		m_andor,
		[&m_andor = m_andor]() {
			m_andor->connectDevice();
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::initCamera() {
	// deinitialize camera if necessary
	if (m_brightfieldCamera) {
		m_brightfieldCamera->deleteLater();
		m_brightfieldCamera = nullptr;
	}

	// initialize correct camera type
	int tabIndexCamera = ui->settingsWidget->indexOf(ui->ODTcameraTab);
	switch (m_cameraType) {
		case CAMERA_DEVICE::NONE:
			m_brightfieldCamera = nullptr;
			ui->actionConnect_Brightfield_camera->setVisible(false);
			if (tabIndexCamera > -1) {
				ui->settingsWidget->removeTab(tabIndexCamera);
			}
			m_hasFluorescence = false;
			break;
		case CAMERA_DEVICE::POINTGREY:
			m_brightfieldCamera = new PointGrey();
			ui->actionConnect_Brightfield_camera->setVisible(true);
			ui->settingsWidget->addTab(ui->ODTcameraTab, "ODT Camera");
			ui->settingsWidget->setTabIcon(3, m_icons.disconnected);
			m_hasFluorescence = true;
			break;
		case CAMERA_DEVICE::UEYE:
			m_brightfieldCamera = new uEyeCam();
			ui->actionConnect_Brightfield_camera->setVisible(true);
			ui->settingsWidget->addTab(ui->ODTcameraTab, "ODT Camera");
			ui->settingsWidget->setTabIcon(3, m_icons.disconnected);
			m_hasFluorescence = true;
			break;
#ifdef _DEBUG
		case CAMERA_DEVICE::MOCK:
			m_brightfieldCamera = new MockCamera();
			ui->actionConnect_Brightfield_camera->setVisible(true);
			ui->settingsWidget->addTab(ui->ODTcameraTab, "ODT Camera");
			ui->settingsWidget->setTabIcon(3, m_icons.disconnected);
			m_hasFluorescence = true;
			break;
#endif
		default:
			m_brightfieldCamera = nullptr;
			ui->actionConnect_Brightfield_camera->setVisible(false);
			if (tabIndexCamera > -1) {
				ui->settingsWidget->removeTab(tabIndexCamera);
			}
			m_hasFluorescence = false;
			break;
	}

	// init or de-init fluorescence
	initFluorescence();

	// don't do anything if no camera is connected
	if (m_cameraType == CAMERA_DEVICE::NONE) {
		return;
	}

	// reestablish camera connections
	QMetaObject::Connection connection = QWidget::connect(
		m_brightfieldCamera,
		&Camera::connectedDevice,
		this,
		[this](bool isConnected) { brightfieldCameraConnectionChanged(isConnected); }
	);

	connection = QWidget::connect(
		m_brightfieldCamera,
		&Camera::s_imageReady,
		this,
		[this] { updateImageODT(); }
	);

	connection = QWidget::connect(
		m_brightfieldCamera,
		&Camera::s_previewBufferSettingsChanged,
		this,
		[this] { updatePlotLimits(m_ODTPlot, m_cameraOptionsODT, m_brightfieldCamera->m_previewBuffer->m_bufferSettings.roi); }
	);

	connection = QWidget::connect(
		m_brightfieldCamera,
		&Camera::s_previewRunning,
		this,
		[this](bool isRunning) { showBrightfieldPreviewRunning(isRunning); }
	);

	connection = QWidget::connect(
		m_brightfieldCamera,
		&Camera::s_acquisitionRunning,
		this,
		[this](bool isRunning) { startBrightfieldPreview(isRunning); }
	);

	connection = QWidget::connect(
		m_brightfieldCamera,
		&Camera::settingsChanged,
		this,
		[this](CAMERA_SETTINGS settings) { cameraODTSettingsChanged(settings); }
	);

	connection = QWidget::connect(
		m_brightfieldCamera,
		&Camera::optionsChanged,
		this,
		[this](CAMERA_OPTIONS options) { cameraODTOptionsChanged(options); }
	);

	m_brightfieldCameraThread.startWorker(m_brightfieldCamera);

	QMetaObject::invokeMethod(
		m_brightfieldCamera,
		[&m_brightfieldCamera = m_brightfieldCamera]() {
			m_brightfieldCamera->connectDevice();
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::microscopeElementPositionsChanged(const std::vector<double>& positions) {
	m_deviceElementPositions = positions;
	checkElementButtons();
}

void BrillouinAcquisition::microscopeElementPositionChanged(DeviceElement element, double position) {
	if (m_deviceElementPositions.size() <= element.index) {
		m_deviceElementPositions.resize((size_t)element.index + 1);
	}
	m_deviceElementPositions[element.index] = position;
	checkElementButtons();
}

void BrillouinAcquisition::checkElementButtons() {
	if ((elementButtons.size() + elementIntBox.size() + elementDoubleBox.size() + elementSlider.size()) != m_deviceElementPositions.size()) {
		return;
	}

	auto elements = m_scanControl->m_deviceElements;
	int indButton{ 0 };
	int indIntBox{ 0 };
	int indDoubleBox{ 0 };
	int indSlider{ 0 };
	for (gsl::index ii = 0; ii < elements.size(); ii++) {
		if (elements[ii].inputType == DEVICE_INPUT_TYPE::PUSHBUTTON) {
			for (gsl::index jj = 0; jj < elementButtons[indButton].size(); jj++) {
				if ((int)m_deviceElementPositions[indButton] == jj + 1) {
					elementButtons[indButton][jj]->setProperty("class", "active");
				} else {
					elementButtons[indButton][jj]->setProperty("class", "");
				}
				elementButtons[indButton][jj]->style()->unpolish(elementButtons[indButton][jj]);
				elementButtons[indButton][jj]->style()->polish(elementButtons[indButton][jj]);
				elementButtons[indButton][jj]->update();
			}
			indButton++;
		} else if (elements[ii].inputType == DEVICE_INPUT_TYPE::INTBOX) {
			const QSignalBlocker blocker(elementIntBox[indIntBox]);
			elementIntBox[indIntBox]->setValue((int)m_deviceElementPositions[ii]);
			indIntBox++;
		} else if (elements[ii].inputType == DEVICE_INPUT_TYPE::DOUBLEBOX) {
			const QSignalBlocker blocker(elementDoubleBox[indDoubleBox]);
			elementDoubleBox[indDoubleBox]->setValue((double)m_deviceElementPositions[ii]);
			indDoubleBox++;
		} else if (elements[ii].inputType == DEVICE_INPUT_TYPE::SLIDER) {
			const QSignalBlocker blocker1(elementSlider[indSlider]);
			elementSlider[indSlider]->setValue((double)m_deviceElementPositions[ii]);

			const QSignalBlocker blocker2(elementSliderInput[indSlider]);
			elementSliderInput[indSlider]->setValue((double)m_deviceElementPositions[ii]);
			indSlider++;
		}
	}
	auto presets = m_scanControl->m_presets;
	for (gsl::index ii = 0; ii < presets.size(); ii++) {
		if (m_scanControl->isPresetActive(presets[ii].index)) {
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
	auto preRelease = QString{ "" };
	if (Version::PRERELEASE.length() > 0) {
		preRelease = QString::fromStdString("-" + Version::PRERELEASE);
	}

	auto debugString = QString{ "" };
	#ifdef _DEBUG
		debugString = QString{ " - Debug" };
	#endif
	QString str = QString("BrillouinAcquisition v%1.%2.%3%4%11 <br> Build from commit: <a href='%5'>%6</a><br>Clean build: %7<br>Author: <a href='mailto:%8?subject=BrillouinAcquisition'>%9</a><br>Date: %10")
		.arg(Version::MAJOR)
		.arg(Version::MINOR)
		.arg(Version::PATCH)
		.arg(preRelease)
		.arg(Version::Url.c_str())
		.arg(Version::Commit.c_str())
		.arg(clean)
		.arg(Version::AuthorEmail.c_str())
		.arg(Version::Author.c_str())
		.arg(Version::Date.c_str())
		.arg(debugString);

	QMessageBox::about(this, tr("About BrillouinAcquisition"), str);
}

void BrillouinAcquisition::on_camera_playPause_clicked() {
	if (!m_andor->m_isPreviewRunning) {
		m_andor->setSettings(m_Brillouin->settings.camera);
		QMetaObject::invokeMethod(
			m_andor,
			[&m_andor = m_andor]() {
				m_andor->startPreview();
			},
			Qt::AutoConnection
		);
	} else {
		m_andor->m_stopPreview = true;
	}
}

void BrillouinAcquisition::on_camera_singleShot_clicked() {
}

void BrillouinAcquisition::on_BrillouinStart_clicked() {
	if (m_Brillouin->getStatus() < ACQUISITION_STATUS::STARTED) {
		// set camera ROI
		m_Brillouin->settings.camera.roi.top = m_deviceSettings.camera.roi.top;
		m_Brillouin->settings.camera.roi.left = m_deviceSettings.camera.roi.left;
		m_Brillouin->settings.camera.roi.width_physical = m_deviceSettings.camera.roi.width_physical;
		m_Brillouin->settings.camera.roi.height_physical = m_deviceSettings.camera.roi.height_physical;
		m_Brillouin->setSettings(m_Brillouin->settings);
		QMetaObject::invokeMethod(
			m_Brillouin,
			[&m_Brillouin = m_Brillouin]() {
				m_Brillouin->startRepetitions();
			},
			Qt::AutoConnection
		);
	} else {
		m_Brillouin->m_abort = true;
	}
}

void BrillouinAcquisition::updateFilename(const std::string& filename) {
	m_storagePath.filename = filename;
	updateBrillouinSettings();
}

void BrillouinAcquisition::updateBrillouinSettings() {
	ui->acquisitionFilename->setText(QString::fromStdString(m_storagePath.filename));

	// AOI settings
	ui->startX->setValue(m_Brillouin->settings.xMin);
	ui->startY->setValue(m_Brillouin->settings.yMin);
	ui->startZ->setValue(m_Brillouin->settings.zMin);
	ui->endX->setValue(m_Brillouin->settings.xMax);
	ui->endY->setValue(m_Brillouin->settings.yMax);
	ui->endZ->setValue(m_Brillouin->settings.zMax);
	ui->stepsX->setValue(m_Brillouin->settings.xSteps);
	ui->stepsY->setValue(m_Brillouin->settings.ySteps);
	ui->stepsZ->setValue(m_Brillouin->settings.zSteps);

	// calibration settings
	ui->preCalibration->setChecked(m_Brillouin->settings.preCalibration);
	ui->postCalibration->setChecked(m_Brillouin->settings.postCalibration);
	ui->conCalibration->setChecked(m_Brillouin->settings.conCalibration);
	ui->conCalibrationInterval->setValue(m_Brillouin->settings.conCalibrationInterval);
	ui->nrCalibrationImages->setValue(m_Brillouin->settings.nrCalibrationImages);
	ui->calibrationExposureTime->setValue(m_Brillouin->settings.calibrationExposureTime);
	ui->sampleSelection->setCurrentText(QString::fromStdString(m_Brillouin->settings.sample));

	// repetition settings
	ui->repetitionCount->setValue(m_Brillouin->settings.repetitions.count);
	ui->repetitionInterval->setValue(m_Brillouin->settings.repetitions.interval);
	ui->repetitionNewFile->setChecked(m_Brillouin->settings.repetitions.filePerRepetition);
}

void BrillouinAcquisition::on_startX_valueChanged(double value) {
	m_Brillouin->setXMin(value);
	updateBrillouinSettings();
}

void BrillouinAcquisition::on_startY_valueChanged(double value) {
	m_Brillouin->setYMin(value);
	updateBrillouinSettings();
}

void BrillouinAcquisition::on_startZ_valueChanged(double value) {
	m_Brillouin->setZMin(value);
	updateBrillouinSettings();
}

void BrillouinAcquisition::on_endX_valueChanged(double value) {
	m_Brillouin->setXMax(value);
	updateBrillouinSettings();
}

void BrillouinAcquisition::on_endY_valueChanged(double value) {
	m_Brillouin->setYMax(value);
	updateBrillouinSettings();
}

void BrillouinAcquisition::on_endZ_valueChanged(double value) {
	m_Brillouin->setZMax(value);
	updateBrillouinSettings();
}

void BrillouinAcquisition::on_stepsX_valueChanged(int value) {
	m_Brillouin->setStepNumberX(value);
	updateBrillouinSettings();
}

void BrillouinAcquisition::on_stepsY_valueChanged(int value) {
	m_Brillouin->setStepNumberY(value);
	updateBrillouinSettings();
}

void BrillouinAcquisition::on_stepsZ_valueChanged(int value) {
	m_Brillouin->setStepNumberZ(value);
	updateBrillouinSettings();
}

void BrillouinAcquisition::on_showOverlay_stateChanged(int show) {
	m_showPositions = show;
	update_AOI_preview();
}

/*
 * React when the ordered positions have changed
 */
void BrillouinAcquisition::AOI_changed(const std::vector<POINT3>& orderedPositions) {
	if (m_scanControl) {
		m_positionsMicrometer = orderedPositions;
		m_positionsPixel = m_scanControl->getPositionsPix(m_positionsMicrometer);
		update_AOI_preview();
	}
}

/*
 * React when the scancontrol settings have changed, e.g. the start position was adjusted
 */
void BrillouinAcquisition::on_scaleCalibrationChanged(const std::vector<POINT2>& positions) {
	m_positionsPixel = positions;
	update_AOI_preview();
}

/*
 * Update the plot showing the measurement positions as overlay in the brightfield preview
 */
void BrillouinAcquisition::update_AOI_preview() {
	if (m_showPositions) {
		QVector<double> xPos(m_positionsPixel.size());
		QVector<double> yPos(m_positionsPixel.size());
		int index{ 0 };
		for (auto const& position : m_positionsPixel) {
			xPos[index] = position.x;
			yPos[index] = position.y;
			++index;
		}
		// Add a marker to the plot to indicate the laser focus
		if (!m_positionsMarker) {
			m_positionsMarker = new QCPCurve(ui->customplot_brightfield->xAxis, ui->customplot_brightfield->yAxis);
			QPen pen;
			pen.setColor(Qt::red);
			pen.setWidth(2);
			QCPScatterStyle scatterStyle;
			scatterStyle.setShape(QCPScatterStyle::ssCross);
			scatterStyle.setPen(pen);
			scatterStyle.setSize(8);
			m_positionsMarker->setScatterStyle(scatterStyle);
		}
		m_positionsMarker->setData(xPos, yPos);
		ui->customplot_brightfield->replot();
	} else if (m_positionsMarker) {
		// Remove the graph and set handle to nullptr if successful
		if (ui->customplot_brightfield->removePlottable(m_positionsMarker)) {
			m_positionsMarker = nullptr;
			ui->customplot_brightfield->replot();
		}
	}
}

void BrillouinAcquisition::on_preCalibration_stateChanged(int state) {
	m_Brillouin->settings.preCalibration = (bool)state;
}

void BrillouinAcquisition::on_postCalibration_stateChanged(int state) {
	m_Brillouin->settings.postCalibration = (bool)state;
}

void BrillouinAcquisition::on_conCalibration_stateChanged(int state) {
	m_Brillouin->settings.conCalibration = (bool)state;
}


void BrillouinAcquisition::on_sampleSelection_currentIndexChanged(const QString &text) {
	m_Brillouin->settings.sample = text.toStdString();
}

void BrillouinAcquisition::on_conCalibrationInterval_valueChanged(double value) {
	m_Brillouin->settings.conCalibrationInterval = value;
}

void BrillouinAcquisition::on_nrCalibrationImages_valueChanged(int value) {
	m_Brillouin->settings.nrCalibrationImages = value;
}

void BrillouinAcquisition::on_calibrationExposureTime_valueChanged(double value) {
	m_Brillouin->settings.calibrationExposureTime = value;
}

/*
 * Functions regarding the repetition feature.
 */

void BrillouinAcquisition::on_repetitionCount_valueChanged(int count) {
	m_Brillouin->settings.repetitions.count = count;
}

void BrillouinAcquisition::on_repetitionInterval_valueChanged(double interval) {
	m_Brillouin->settings.repetitions.interval = interval;
}

void BrillouinAcquisition::on_repetitionNewFile_stateChanged(int checked) {
	m_Brillouin->settings.repetitions.filePerRepetition = (bool)checked;
}

void BrillouinAcquisition::showRepProgress(int repNumber, int timeToNext) {
	ui->repetitionProgress->setValue(100 * ((double)repNumber + 1) / m_Brillouin->settings.repetitions.count);

	QString string;
	if (timeToNext > 0) {
		string = formatSeconds(timeToNext) + " to next repetition.";
	} else if (timeToNext > -2) {
		if (repNumber < m_Brillouin->settings.repetitions.count) {
			string.sprintf("Measuring repetition %1.0d of %1.0d.", repNumber + 1, m_Brillouin->settings.repetitions.count);
		} else {
			string.sprintf("Finished %1.0d repetitions.", m_Brillouin->settings.repetitions.count);
		}
	} else {
		string.sprintf("Finished %1.0d repetitions.", repNumber);
	}
	ui->repetitionProgress->setFormat(string);
}

void BrillouinAcquisition::on_savePosition_clicked() {
	QMetaObject::invokeMethod(
		m_scanControl,
		[&m_scanControl = m_scanControl]() {
			m_scanControl->savePosition();
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::on_setHome_clicked() {
	QMetaObject::invokeMethod(
		m_scanControl,
		[&m_scanControl = m_scanControl]() {
			m_scanControl->setHome();
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::on_moveHome_clicked() {
	if (m_enabledModes == ACQUISITION_MODE::NONE) {
		QMetaObject::invokeMethod(
			m_scanControl,
			[&m_scanControl = m_scanControl]() {
				m_scanControl->moveHome();
			},
			Qt::AutoConnection
		);
	}
}

void BrillouinAcquisition::on_setPositionX_valueChanged(double positionX) {
	if (m_enabledModes == ACQUISITION_MODE::NONE) {
		QMetaObject::invokeMethod(
			m_scanControl,
			[&m_scanControl = m_scanControl, positionX]() {
				m_scanControl->setPositionRelativeX(positionX);
			},
			Qt::AutoConnection
		);
	}
}

void BrillouinAcquisition::on_setPositionY_valueChanged(double positionY) {
	if (m_enabledModes == ACQUISITION_MODE::NONE) {
		QMetaObject::invokeMethod(
			m_scanControl,
			[&m_scanControl = m_scanControl, positionY]() {
				m_scanControl->setPositionRelativeY(positionY);
			},
			Qt::AutoConnection
		);
	}
}

void BrillouinAcquisition::on_setPositionZ_valueChanged(double positionZ) {
	if (m_enabledModes == ACQUISITION_MODE::NONE) {
		QMetaObject::invokeMethod(
			m_scanControl,
			[&m_scanControl = m_scanControl, positionZ]() {
				m_scanControl->setPositionRelativeZ(positionZ);
			},
			Qt::AutoConnection
		);
	}
}

void BrillouinAcquisition::updateSavedPositions() {
	ui->tableView->setModel(tableModel);
	ui->tableView->setItemDelegateForColumn(3, &buttonDelegate);
	ui->tableView->verticalHeader()->setDefaultSectionSize(22);
	ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui->tableView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	ui->tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	ui->tableView->show();
}

void BrillouinAcquisition::on_scanDirAutoCheckbox_stateChanged(int automatical) {
	m_Brillouin->setScanOrderAuto((bool)automatical);
}

void BrillouinAcquisition::on_buttonGroup_buttonClicked(int button) {
	m_Brillouin->setScanOrderX(button);
}

void BrillouinAcquisition::on_buttonGroup_2_buttonClicked(int button) {
	m_Brillouin->setScanOrderY(button);
}

void BrillouinAcquisition::on_buttonGroup_3_buttonClicked(int button) {
	m_Brillouin->setScanOrderZ(button);
}

void BrillouinAcquisition::scanOrderChanged(SCAN_ORDER scanOrder) {
	if (scanOrder.x == 0) {
		ui->scanDirX0->setChecked(true);
	}
	if (scanOrder.x == 1) {
		ui->scanDirX1->setChecked(true);
	}
	if (scanOrder.x == 2) {
		ui->scanDirX2->setChecked(true);
	}
	if (scanOrder.y == 0) {
		ui->scanDirY0->setChecked(true);
	}
	if (scanOrder.y == 1) {
		ui->scanDirY1->setChecked(true);
	}
	if (scanOrder.y == 2) {
		ui->scanDirY2->setChecked(true);
	}
	if (scanOrder.z == 0) {
		ui->scanDirZ0->setChecked(true);
	}
	if (scanOrder.z == 1) {
		ui->scanDirZ1->setChecked(true);
	}
	if (scanOrder.z == 2) {
		ui->scanDirZ2->setChecked(true);
	}
	// disable radio buttons if order is determined automatically
	ui->scanDirX0->setDisabled(scanOrder.automatical);
	ui->scanDirX1->setDisabled(scanOrder.automatical);
	ui->scanDirX2->setDisabled(scanOrder.automatical);
	ui->scanDirY0->setDisabled(scanOrder.automatical);
	ui->scanDirY1->setDisabled(scanOrder.automatical);
	ui->scanDirY2->setDisabled(scanOrder.automatical);
	ui->scanDirZ0->setDisabled(scanOrder.automatical);
	ui->scanDirZ1->setDisabled(scanOrder.automatical);
	ui->scanDirZ2->setDisabled(scanOrder.automatical);
}

void BrillouinAcquisition::on_exposureTime_valueChanged(double value) {
	m_Brillouin->settings.camera.exposureTime = value;
}

void BrillouinAcquisition::on_frameCount_valueChanged(int value) {
	m_Brillouin->settings.camera.frameCount = value;
}

StoragePath BrillouinAcquisition::splitFilePath(QString fullPath) {
	QFileInfo fileInfo(fullPath);
	return {
		fileInfo.fileName().toStdString(),
		fileInfo.absolutePath().toStdString()
	};
}

QString BrillouinAcquisition::checkFilename(QString absoluteFilePath) {
	QFileInfo fileInfo(absoluteFilePath);
	// get filename without extension
	std::string rawFilename = fileInfo.baseName().toStdString();
	// remove possibly attached number separated by a hyphen
	rawFilename = rawFilename.substr(0, rawFilename.find_last_of("-"));
	int count = 0;
	std::string filename;
	std::string fullPath = absoluteFilePath.toStdString();
	while (exists(fullPath)) {
		filename = rawFilename + '-' + std::to_string(count) + '.' + fileInfo.completeSuffix().toStdString();
		fullPath = fileInfo.absolutePath().toStdString() + "/" + filename;
		count++;
	}
	return QString::fromStdString(fullPath);
}

void BrillouinAcquisition::on_actionNew_Acquisition_triggered() {

	StoragePath tmpStorage = m_storagePath;

	if (tmpStorage.filename.length() == 0) {
		tmpStorage.filename = StoragePath{}.filename;
	}

	QString proposedFileName = QString::fromStdString(tmpStorage.fullPath());

	proposedFileName = checkFilename(proposedFileName);

	// TODO: Check if filename already exsists and increment accordingly

	QString fullPath = QFileDialog::getSaveFileName(this, tr("Save new Acquisition as"),
		proposedFileName, tr("Brillouin data (*.h5)"));

	if (fullPath.isEmpty()) {
		return;
	}

	m_storagePath = splitFilePath(fullPath);

	QMetaObject::invokeMethod(
		m_acquisition,
		[&m_acquisition = m_acquisition, &m_storagePath = m_storagePath]() {
			m_acquisition->newFile(m_storagePath);
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::on_actionOpen_Acquisition_triggered() {
	QString fullPath = QFileDialog::getOpenFileName(this, tr("Save new Acquisition as"),
		QString::fromStdString(m_storagePath.folder), tr("Brillouin data (*.h5)"));

	if (fullPath.isEmpty()) {
		return;
	}

	m_storagePath = splitFilePath(fullPath);

	QMetaObject::invokeMethod(
		m_acquisition,
		[&m_acquisition = m_acquisition, &m_storagePath = m_storagePath]() {
			m_acquisition->openFile(m_storagePath);
		},
		Qt::AutoConnection
	);
}

void BrillouinAcquisition::on_actionClose_Acquisition_triggered() {
	int ret = m_acquisition->closeFile();
	if (ret == 0) {
		m_storagePath.filename = "";
		ui->acquisitionFilename->setText(QString::fromStdString(m_storagePath.filename));
	}
}

void BrillouinAcquisition::setColormap(QCPColorGradient *gradient, const CustomGradientPreset& preset) {
	gradient->clearColorStops();
	switch (preset) {
		case CustomGradientPreset::gpViridis: {
			gradient->setColorInterpolation(QCPColorGradient::ciRGB);
			BrillouinAcquisition::applyColorMap(gradient, ColorMaps::m_viridis);
			break;
		}
		case CustomGradientPreset::gpGrayscale:
			gradient->loadPreset(QCPColorGradient::gpGrayscale);
			break;
		case CustomGradientPreset::gpInferno:
			gradient->setColorInterpolation(QCPColorGradient::ciRGB);
			BrillouinAcquisition::applyColorMap(gradient, ColorMaps::m_inferno);
			break;
		default:
			gradient->loadPreset(QCPColorGradient::gpGrayscale);
			break;
	}
}

void BrillouinAcquisition::applyColorMap(QCPColorGradient* gradient, const std::vector<std::vector<double>>& colorMap) {
	for (gsl::index index{ 0 }; auto& color : colorMap) {
		gradient->setColorStopAt((double)index++ / colorMap.size(), QColor((int)255 * color[0], (int)255 * color[1], (int)255 * color[2]));
	}
}

void BrillouinAcquisition::writeSettings() {
	QSettings settings(QSettings::IniFormat, QSettings::UserScope,
		"Guck Lab", "Brillouin Acquisition");

	auto brillouinCamera = QString{};
	switch (m_cameraBrillouinType) {
		case CAMERA_BRILLOUIN_DEVICE::ANDOR:
			brillouinCamera = "andor";
			break;
		case CAMERA_BRILLOUIN_DEVICE::PVCAM:
			brillouinCamera = "pvcam";
			break;
#ifdef _DEBUG
		case CAMERA_BRILLOUIN_DEVICE::MOCK:
			brillouinCamera = "mock";
			break;
#endif
		default:
			brillouinCamera = "andor";
			break;
	}
	auto brillouinCameraNumber = QString::number(m_cameraBrillouinNumber);

	auto brightfieldCamera = QString{};
	switch (m_cameraType) {
		case CAMERA_DEVICE::UEYE:
			brightfieldCamera = "ueye";
			break;
		case CAMERA_DEVICE::POINTGREY:
			brightfieldCamera = "pointgrey";
			break;
#ifdef _DEBUG
		case CAMERA_DEVICE::MOCK:
			brightfieldCamera = "mock";
			break;
#endif
		default:
			brightfieldCamera = "none";
			break;
	}

	auto stage = QString{};
	switch (m_scanControllerType) {
		case ScanControl::SCAN_DEVICE::ZEISSECU:
			stage = "zeiss-ecu";
			break;
		case ScanControl::SCAN_DEVICE::NIDAQ:
			stage = "nidaq";
			break;
		case ScanControl::SCAN_DEVICE::ZEISSMTB:
			stage = "zeiss-mtb";
			break;
		case ScanControl::SCAN_DEVICE::ZEISSMTBERLANGEN:
			stage = "zeiss-mtb-erlangen";
			break;
		case ScanControl::SCAN_DEVICE::ZEISSMTBERLANGEN2:
			stage = "zeiss-mtb-erlangen-2";
			break;
		default:
			stage = "zeiss-ecu";
			break;
	}

	settings.beginGroup("devices");
	settings.setValue("brillouin-camera", brillouinCamera);
	settings.setValue("brillouin-camera-number", brillouinCameraNumber);
	settings.setValue("brightfield-camera", brightfieldCamera);
	settings.setValue("stage", stage);
	settings.endGroup();
	settings.beginGroup("scale-calibration");
	settings.setValue("file-path", QString::fromStdString(m_scaleCalibrationFilePath));
	settings.endGroup();
	settings.beginGroup("devices-settings");
	settings.setValue("stage-laser-position-x", m_positionScanner.x);
	settings.setValue("stage-laser-position-y", m_positionScanner.y);
	settings.setValue("stage-x-min", m_Brillouin->settings.xMin);
	settings.setValue("stage-x-max", m_Brillouin->settings.xMax);
	settings.setValue("stage-x-steps", m_Brillouin->settings.xSteps);
	settings.setValue("stage-y-min", m_Brillouin->settings.yMin);
	settings.setValue("stage-y-max", m_Brillouin->settings.yMax);
	settings.setValue("stage-y-steps", m_Brillouin->settings.ySteps);
	settings.setValue("stage-z-min", m_Brillouin->settings.zMin);
	settings.setValue("stage-z-max", m_Brillouin->settings.zMax);
	settings.setValue("stage-z-steps", m_Brillouin->settings.zSteps);
	settings.setValue("brillouin-pre-calibrate", m_Brillouin->settings.preCalibration);
	settings.setValue("brillouin-post-calibrate", m_Brillouin->settings.postCalibration);
	settings.setValue("brillouin-con-calibrate", m_Brillouin->settings.conCalibration);
	settings.setValue("brillouin-con-calibrate-interval", m_Brillouin->settings.conCalibrationInterval);
	settings.setValue("brillouin-nr-calibration-images", m_Brillouin->settings.nrCalibrationImages);
	settings.setValue("brillouin-calibration-exposure-time", m_Brillouin->settings.calibrationExposureTime);
	settings.setValue("brillouin-camera-roi-left", m_deviceSettings.camera.roi.left);
	settings.setValue("brillouin-camera-roi-top", m_deviceSettings.camera.roi.top);
	settings.setValue("brillouin-camera-roi-width-physical", m_deviceSettings.camera.roi.width_physical);
	settings.setValue("brillouin-camera-roi-height-physical", m_deviceSettings.camera.roi.height_physical);
	settings.setValue("brillouin-camera-exposure-time", m_Brillouin->settings.camera.exposureTime);
	settings.setValue("brillouin-camera-frame-count", m_Brillouin->settings.camera.frameCount);
	settings.endGroup();
}

void BrillouinAcquisition::readSettings() {
	QSettings settings(QSettings::IniFormat, QSettings::UserScope,
		"Guck Lab", "Brillouin Acquisition");

	settings.beginGroup("devices");
	QVariant BrillouinCam = settings.value("brillouin-camera");
	QVariant BrillouinCamNumber = settings.value("brillouin-camera-number");
	QVariant BrightfieldCam = settings.value("brightfield-camera");
	QVariant stage = settings.value("stage");

	// Brillouin camera
	if (BrillouinCam == "andor") {
		m_cameraBrillouinType = CAMERA_BRILLOUIN_DEVICE::ANDOR;
	} else if (BrillouinCam == "pvcam") {
		m_cameraBrillouinType = CAMERA_BRILLOUIN_DEVICE::PVCAM;
	}
#ifdef _DEBUG
	else if (BrillouinCam == "mock") {
		m_cameraBrillouinType = CAMERA_BRILLOUIN_DEVICE::MOCK;
	}
#endif
	else {
		m_cameraBrillouinType = CAMERA_BRILLOUIN_DEVICE::ANDOR;
	}

	m_cameraBrillouinNumber = BrillouinCamNumber.toInt();

	// Brightfield camera
	if (BrightfieldCam == "ueye") {
		m_cameraType = CAMERA_DEVICE::UEYE;
	} else if (BrightfieldCam == "pointgrey") {
		m_cameraType = CAMERA_DEVICE::POINTGREY;
	}
#ifdef _DEBUG
	else if (BrightfieldCam == "mock") {
		m_cameraType = CAMERA_DEVICE::MOCK;
	}
#endif
	else {
		m_cameraType = CAMERA_DEVICE::NONE;
	}

	// Scanning stage
	if (stage == "zeiss-ecu") {
		m_scanControllerType = ScanControl::SCAN_DEVICE::ZEISSECU;
	} else if (stage == "nidaq") {
		m_scanControllerType = ScanControl::SCAN_DEVICE::NIDAQ;
	} else if (stage == "zeiss-mtb") {
		m_scanControllerType = ScanControl::SCAN_DEVICE::ZEISSMTB;
	} else if (stage == "zeiss-mtb-erlangen") {
		m_scanControllerType = ScanControl::SCAN_DEVICE::ZEISSMTBERLANGEN;
	} else if (stage == "zeiss-mtb-erlangen-2") {
		m_scanControllerType = ScanControl::SCAN_DEVICE::ZEISSMTBERLANGEN2;
	} else {
		m_scanControllerType = ScanControl::SCAN_DEVICE::ZEISSECU;
	}

	settings.endGroup();

	settings.beginGroup("scale-calibration");
	QVariant filePath = settings.value("file-path");
	m_scaleCalibrationFilePath = filePath.toString().toStdString();
	settings.endGroup();

	settings.beginGroup("devices-settings");
	auto posX = settings.value("stage-laser-position-x");
	auto posY = settings.value("stage-laser-position-y");
	m_positionScanner = POINT2(posX.toDouble(), posY.toDouble());
	m_Brillouin->settings.setXMin(settings.value("stage-x-min", m_Brillouin->settings.xMin).toInt());
	m_Brillouin->settings.setXMax(settings.value("stage-x-max", m_Brillouin->settings.xMax).toInt());
	m_Brillouin->settings.setXSteps(settings.value("stage-x-steps", m_Brillouin->settings.xSteps).toInt());
	m_Brillouin->settings.setYMin(settings.value("stage-y-min", m_Brillouin->settings.yMin).toInt());
	m_Brillouin->settings.setYMax(settings.value("stage-y-max", m_Brillouin->settings.yMax).toInt());
	m_Brillouin->settings.setYSteps(settings.value("stage-y-steps", m_Brillouin->settings.ySteps).toInt());
	m_Brillouin->settings.setZMin(settings.value("stage-z-min", m_Brillouin->settings.zMin).toInt());
	m_Brillouin->settings.setZMax(settings.value("stage-z-max", m_Brillouin->settings.zMax).toInt());
	m_Brillouin->settings.setZSteps(settings.value("stage-z-steps", m_Brillouin->settings.zSteps).toInt());
	m_Brillouin->settings.preCalibration = settings.value("brillouin-pre-calibrate", m_Brillouin->settings.preCalibration).toBool();
	m_Brillouin->settings.postCalibration = settings.value("brillouin-post-calibrate", m_Brillouin->settings.postCalibration).toBool();
	m_Brillouin->settings.conCalibration = settings.value("brillouin-con-calibrate", m_Brillouin->settings.conCalibration).toBool();
	m_Brillouin->settings.conCalibrationInterval = settings.value("brillouin-con-calibrate-interval", m_Brillouin->settings.conCalibrationInterval).toDouble();
	m_Brillouin->settings.nrCalibrationImages = settings.value("brillouin-nr-calibration-images", m_Brillouin->settings.nrCalibrationImages).toInt();
	m_Brillouin->settings.calibrationExposureTime = settings.value("brillouin-calibration-exposure-time", m_Brillouin->settings.calibrationExposureTime).toDouble();
	settings.endGroup();
}