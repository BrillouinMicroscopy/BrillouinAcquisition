#include "stdafx.h"
#include "ODT.h"
#include "../../simplemath.h"

/*
 * Public definitions
 */

ODT::ODT(QObject* parent, Acquisition* acquisition, Camera** camera, ODTControl** ODTControl)
	: AcquisitionMode(parent, acquisition, (ScanControl**)ODTControl), m_camera(camera), m_ODTControl(ODTControl) {
}

ODT::~ODT() {
	m_algnRunning = false;
	if (m_algnTimer) {
		m_algnTimer->stop();
		m_algnTimer->deleteLater();
		m_algnTimer = nullptr;
	}
}

bool ODT::isAlgnRunning() {
	return m_algnRunning;
}

void ODT::setAlgnSettings(ODT_SETTINGS settings) {
	m_algnSettings = settings;
	calculateVoltages(ODT_MODE::ALGN);
}

void ODT::setSettings(ODT_SETTINGS settings) {
	m_acqSettings = settings;
	calculateVoltages(ODT_MODE::ACQ);
}

/*
 * Public slots
 */

void ODT::startRepetitions() {
	bool allowed = m_acquisition->enableMode(ACQUISITION_MODE::ODT);
	if (!allowed) {
		return;
	}

	// stop alignment if it is running
	if (m_algnTimer->isActive()) {
		m_algnRunning = false;
		m_algnTimer->stop();
	}

	// reset abort flag
	m_abort = false;

	// configure camera for measurement
	CAMERA_SETTINGS settings = (*m_camera)->getSettings();
	// set ROI and readout parameters to default ODT values, exposure time and gain will be kept
	settings.roi.left = 128;
	settings.roi.top = 0;
	settings.roi.width_physical = 1024;
	settings.roi.height_physical = 1024;
	settings.readout.triggerMode = L"External";
	settings.readout.cycleMode = L"Continuous";
	settings.frameCount = m_acqSettings.numberPoints;
	settings.exposureTime = m_cameraSettings.exposureTime;
	settings.gain = m_cameraSettings.gain;

	(*m_camera)->startAcquisition(settings);

	// read back the applied settings
	m_cameraSettings = (*m_camera)->getSettings();

	m_acquisition->newRepetition(ACQUISITION_MODE::ODT);

	// start repetition
	acquire(m_acquisition->m_storage);

	// Center laser beam after measuremenet
	centerAlignment();

	// configure camera for preview
	(*m_camera)->stopAcquisition();

	m_acquisition->disableMode(ACQUISITION_MODE::ODT);
}

void ODT::init() {
	m_algnTimer = new QTimer();
	QMetaObject::Connection connection = QWidget::connect(
		m_algnTimer,
		&QTimer::timeout,
		this,
		&ODT::nextAlgnPosition
	);
}

void ODT::initialize() {
	calculateVoltages(ODT_MODE::ALGN);
	calculateVoltages(ODT_MODE::ACQ);
	emit(s_cameraSettingsChanged(m_cameraSettings));
}

void ODT::startAlignment() {
	if (!m_algnRunning) {
		bool allowed = m_acquisition->enableMode(ACQUISITION_MODE::ODT);
		if (!allowed) {
			return;
		}
		setAcquisitionStatus(ACQUISITION_STATUS::ALIGNING);
		m_algnRunning = true;

		// configure camera exposure and gain
		CAMERA_SETTINGS settings = (*m_camera)->getSettings();
		settings.exposureTime = m_cameraSettings.exposureTime;
		settings.gain = m_cameraSettings.gain;
		QMetaObject::invokeMethod(
			(*m_camera),
			[&m_camera = (*m_camera), settings]() { m_camera->setSettings(settings); }, Qt::AutoConnection
		);

		// move to ODT configuration
		(*m_ODTControl)->setPreset(ScanPreset::SCAN_ODT);
		// stop querying the element positions, because querying the filter mounts block the thread quite long
		(*m_ODTControl)->stopAnnouncingElementPosition();
		// start the timer
		if (!m_algnTimer->isActive()) {
			m_algnTimer->start(1e3 / (m_algnSettings.scanRate * m_algnSettings.numberPoints));
		}
	}
	else {
		m_algnRunning = false;
		if (m_algnTimer->isActive()) {
			m_algnTimer->stop();
		}
		// start querying the element positions again
		(*m_ODTControl)->startAnnouncingElementPosition();
		m_acquisition->disableMode(ACQUISITION_MODE::ODT);
		setAcquisitionStatus(ACQUISITION_STATUS::STOPPED);
	}
}

void ODT::centerAlignment() {
	if (!m_algnRunning && (m_status < ACQUISITION_STATUS::RUNNING)) {
		(*m_ODTControl)->setVoltage({ 0, 0 });

		// announce mirror voltage
		emit(s_mirrorVoltageChanged({ 0, 0 }, ODT_MODE::ALGN));
	}
}

void ODT::setSettings(ODT_MODE mode, ODT_SETTING settingType, double value) {
	ODT_SETTINGS *settings;
	if (mode == ODT_MODE::ACQ) {
		settings = &m_acqSettings;
	} else if (mode == ODT_MODE::ALGN) {
		settings = &m_algnSettings;
	} else {
		return;
	}

	switch (settingType) {
		case ODT_SETTING::VOLTAGE:
			settings->radialVoltage = value;
			calculateVoltages(mode);
			break;
		case ODT_SETTING::NRPOINTS:
			settings->numberPoints = value;
			calculateVoltages(mode);
			if (mode == ODT_MODE::ALGN && m_algnRunning) {
				m_algnTimer->start(1e3 / (m_algnSettings.scanRate * m_algnSettings.numberPoints));
			}
			break;
		case ODT_SETTING::SCANRATE:
			settings->scanRate = value;
			if (mode == ODT_MODE::ALGN && m_algnRunning) {
				m_algnTimer->start(1e3 / (m_algnSettings.scanRate * m_algnSettings.numberPoints));
			}
			break;
	}
}

void ODT::setCameraSetting(CAMERA_SETTING type, double value) {
	switch (type) {
		case CAMERA_SETTING::EXPOSURE:
			m_cameraSettings.exposureTime = value;
			break;
		case CAMERA_SETTING::GAIN:
			m_cameraSettings.gain = value;
			break;
	}
	
	// Apply camera settings immediately if alignment is running
	if (m_algnRunning) {
		QMetaObject::invokeMethod(
			(*m_camera),
			[&m_camera = (*m_camera), type, value]() { m_camera->setSetting(type, value); },
			Qt::AutoConnection
		);
	}

	emit(s_cameraSettingsChanged(m_cameraSettings));
}

/*
 * Private definitions
 */

void ODT::abortMode(std::unique_ptr <StorageWrapper>& storage) {
	// Here we wait until the storage object indicate it finished to write to the file.
	QEventLoop loop;
	auto connection = QWidget::connect(
		storage.get(),
		&StorageWrapper::finished,
		&loop,
		&QEventLoop::quit
	);
	QMetaObject::invokeMethod(
		storage.get(),
		[&storage = storage]() { storage.get()->s_finishedQueueing(); },
		Qt::AutoConnection
	);
	loop.exec();

	abortMode();
}

void ODT::abortMode() {
	// stop alignment if it is running
	if (m_algnTimer->isActive()) {
		m_algnRunning = false;
		m_algnTimer->stop();
	}
	m_acquisition->disableMode(ACQUISITION_MODE::ODT);
	setAcquisitionStatus(ACQUISITION_STATUS::ABORTED);
}

void ODT::calculateVoltages(ODT_MODE mode) {
	if (mode == ODT_MODE::ALGN) {
		double Ux{ 0 };
		double Uy{ 0 };
		std::vector<double> theta = simplemath::linspace<double>(0, 360, (size_t)m_algnSettings.numberPoints + 1);
		theta.erase(theta.end() - 1);
		m_algnSettings.voltages.resize(theta.size());
		for (gsl::index i{ 0 }; i < theta.size(); i++) {
			Ux = m_algnSettings.radialVoltage * cos(theta[i] * M_PI / 180);
			Uy = m_algnSettings.radialVoltage * sin(theta[i] * M_PI / 180);
			m_algnSettings.voltages[i] = { Ux, Uy };
		}
		emit(s_algnSettingsChanged(m_algnSettings));
	}
	if (mode == ODT_MODE::ACQ) {
		m_acqSettings.voltages.clear();
		if (m_acqSettings.numberPoints < 10) {
			return;
		}
		double Ux{ 0 };
		double Uy{ 0 };

		int n3 = round(m_acqSettings.numberPoints / 3);

		std::vector<double> theta = simplemath::linspace<double>(2 * M_PI, 0, n3);
		theta.erase(theta.begin());
		for (gsl::index i{ 0 }; i < theta.size(); i++) {

			double r = sqrt(abs(theta[i]));
			Ux = m_acqSettings.radialVoltage * r * cos(theta[i]) / sqrt(2 * M_PI);
			Uy = m_acqSettings.radialVoltage * r * sin(theta[i]) / sqrt(2 * M_PI);

			m_acqSettings.voltages.push_back({ Ux, Uy });
		}

		theta = simplemath::linspace<double>(0, 2 * M_PI, n3);
		theta.erase(theta.begin());
		theta.erase(theta.end() - 1);
		for (gsl::index i{ 0 }; i < theta.size(); i++) {

			double r = sqrt(abs(theta[i]));
			Ux = m_acqSettings.radialVoltage * -r * cos(theta[i]) / sqrt(2 * M_PI);
			Uy = m_acqSettings.radialVoltage * -r * sin(theta[i]) / sqrt(2 * M_PI);

			m_acqSettings.voltages.push_back({ Ux, Uy });
		}

		theta = simplemath::linspace<double>(0, 2 * M_PI, m_acqSettings.numberPoints - (size_t)2 * n3 + 3);
		theta = simplemath::linspace<double>(0, theta.end()[-2], m_acqSettings.numberPoints - (size_t)2 * n3 + 3);
		for (gsl::index i{ 0 }; i < theta.size(); i++) {

			Ux = -1 * m_acqSettings.radialVoltage * cos(theta[i]);
			Uy = -1 * m_acqSettings.radialVoltage * sin(theta[i]);

			m_acqSettings.voltages.push_back({ Ux, Uy });
		}

		m_acqSettings.numberPoints = m_acqSettings.voltages.size();

		emit(s_acqSettingsChanged(m_acqSettings));
	}
}

std::string ODT::getBinningString() {
	std::string binning{ "1x1" };
	if (m_cameraSettings.roi.binning == L"8x8") {
		binning = "8x8";
	} else if (m_cameraSettings.roi.binning == L"4x4") {
		binning = "4x4";
	} else if (m_cameraSettings.roi.binning == L"2x2") {
		binning = "2x2";
	}
	return binning;
}

template <typename T>
void ODT::__acquire(std::unique_ptr <StorageWrapper> & storage) {
	setAcquisitionStatus(ACQUISITION_STATUS::STARTED);

	QMetaObject::invokeMethod(
		storage.get(),
		[&storage = storage]() { storage.get()->startWritingQueues(); },
		Qt::AutoConnection
	);

	// move to ODT configuration
	(*m_ODTControl)->setPreset(ScanPreset::SCAN_ODT);

	// Set first mirror voltage already
	(*m_ODTControl)->setVoltage(m_acqSettings.voltages[0]);
	Sleep(100);

	writeScaleCalibration(storage, ACQUISITION_MODE::ODT);

	ACQ_VOLTAGES voltages;

	/*
	 * Set the mirror voltage and trigger the camera
	 */
	// Construct the analog voltage vector and the trigger vector
	int samplesPerAngle{ 10 };
	int numberChannels{ 2 };
	voltages.numberSamples = m_acqSettings.numberPoints * samplesPerAngle;
	voltages.trigger = std::vector<uInt8>((size_t)m_acqSettings.numberPoints * samplesPerAngle, 0);
	voltages.mirror = std::vector<float64>((size_t)m_acqSettings.numberPoints * samplesPerAngle * numberChannels, 0);
	for (gsl::index i{ 0 }; i < m_acqSettings.numberPoints; i++) {
		voltages.trigger[i * samplesPerAngle + 2] = 1;
		voltages.trigger[i * samplesPerAngle + 3] = 1;
		std::fill_n(voltages.mirror.begin() + i * samplesPerAngle, samplesPerAngle, m_acqSettings.voltages[i].Ux);
		std::fill_n(voltages.mirror.begin() + i * samplesPerAngle + (size_t)m_acqSettings.numberPoints * samplesPerAngle, samplesPerAngle, m_acqSettings.voltages[i].Uy);
	}
	// Apply voltages to NIDAQ board
	(*m_ODTControl)->setAcquisitionVoltages(voltages);

	int rank_data{ 3 };
	hsize_t dims_data[3] = { 1, (hsize_t)m_cameraSettings.roi.height_binned, (hsize_t)m_cameraSettings.roi.width_binned };
	std::string binning = getBinningString();
	if (m_cameraSettings.roi.bytesPerFrame) {
		for (gsl::index i{ 0 }; i < m_acqSettings.numberPoints; i++) {

			// read images from camera
			std::vector<std::byte> images(m_cameraSettings.roi.bytesPerFrame);

			if (m_abort) {
				this->abortMode(storage);
				return;
			}

			// acquire images
			(*m_camera)->getImageForAcquisition(&images[0], false);


			// store images
			// asynchronously write image to disk
			// the datetime has to be set here, otherwise it would be determined by the time the queue is processed
			std::string date = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
				.toString(Qt::ISODateWithMs).toStdString();

			// cast the image to type T
			auto images_ = (std::vector<T>*) & images;
			auto img = new ODTIMAGE<T>((int)i, rank_data, dims_data, date, *images_,
				m_cameraSettings.exposureTime, m_cameraSettings.gain, binning);

			QMetaObject::invokeMethod(
				storage.get(),
				[&storage = storage, img]() { storage.get()->s_enqueuePayload(img); },
				Qt::AutoConnection
			);
		}
	}

	// Here we wait until the storage object indicate it finished to write to the file.
	QEventLoop loop;
	auto connection = QWidget::connect(
		storage.get(),
		&StorageWrapper::finished,
		&loop,
		&QEventLoop::quit
	);
	QMetaObject::invokeMethod(
		storage.get(),
		[&storage = storage]() { storage.get()->s_finishedQueueing(); },
		Qt::AutoConnection
	);
	loop.exec();

	setAcquisitionStatus(ACQUISITION_STATUS::FINISHED);
}

/*
 * Private slots
 */
void ODT::acquire(std::unique_ptr <StorageWrapper>& storage) {
	if (m_cameraSettings.readout.dataType == "unsigned short") {
		__acquire<unsigned short>(storage);
	} else if (m_cameraSettings.readout.dataType == "unsigned char") {
		__acquire<unsigned char>(storage);
	}
}

void ODT::nextAlgnPosition() {
	if (m_abortAlignment) {
		this->abortMode();
		return;
	}
	if (++m_algnPositionIndex >= m_algnSettings.numberPoints) {
		m_algnPositionIndex = 0;
	}
	VOLTAGE2 voltage = m_algnSettings.voltages[m_algnPositionIndex];
	// set new voltage to galvo mirrors
	(*m_ODTControl)->setVoltage(voltage);

	// announce mirror voltage
	emit(s_mirrorVoltageChanged(voltage, ODT_MODE::ALGN));
}
