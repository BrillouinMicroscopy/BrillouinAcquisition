#include "stdafx.h"
#include "ODT.h"
#include "../../simplemath.h"

ODT::ODT(QObject* parent, Acquisition* acquisition, Camera** camera, NIDAQ** nidaq)
	: AcquisitionMode(parent, acquisition), m_camera(camera), m_NIDAQ(nidaq) {
}

ODT::~ODT() {
	m_algnRunning = false;
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
		QMetaObject::invokeMethod((*m_camera), "setSetting", Qt::AutoConnection, Q_ARG(CAMERA_SETTING, type), Q_ARG(double, value));
	}

	emit(s_cameraSettingsChanged(m_cameraSettings));
}

void ODT::init() {
	m_algnTimer = new QTimer();
	QMetaObject::Connection connection = QWidget::connect(m_algnTimer, SIGNAL(timeout()), this, SLOT(nextAlgnPosition()));
}

void ODT::initialize() {
	calculateVoltages(ODT_MODE::ALGN);
	calculateVoltages(ODT_MODE::ACQ);
	emit(s_cameraSettingsChanged(m_cameraSettings));
}

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
	settings.roi.width = 1024;
	settings.roi.height = 1024;
	settings.readout.pixelEncoding = L"Raw8";
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

	// configure camera for preview
	(*m_camera)->stopAcquisition();

	m_acquisition->disableMode(ACQUISITION_MODE::ODT);
}

void ODT::acquire(std::unique_ptr <StorageWrapper> & storage) {
	m_status = ACQUISITION_STATUS::STARTED;
	emit(s_acquisitionStatus(m_status));

	// move to ODT configuration
	(*m_NIDAQ)->setPreset(SCAN_ODT);

	// Set first mirror voltage already
	(*m_NIDAQ)->setVoltage(m_acqSettings.voltages[0]);
	Sleep(100);

	ACQ_VOLTAGES voltages;

	/*
	 * Set the mirror voltage and trigger the camera
	 */
	// Construct the analog voltage vector and the trigger vector
	int samplesPerAngle{ 10 };
	int numberChannels{ 2 };
	voltages.numberSamples = m_acqSettings.numberPoints * samplesPerAngle;
	voltages.trigger = std::vector<uInt8>(m_acqSettings.numberPoints * samplesPerAngle, 0);
	voltages.mirror = std::vector<float64>(m_acqSettings.numberPoints * samplesPerAngle * numberChannels, 0);
	for (gsl::index i{ 0 }; i < m_acqSettings.numberPoints; i++) {
		voltages.trigger[i * samplesPerAngle + 2] = 1;
		voltages.trigger[i * samplesPerAngle + 3] = 1;
		std::fill_n(voltages.mirror.begin() + i * samplesPerAngle, samplesPerAngle, m_acqSettings.voltages[i].Ux);
		std::fill_n(voltages.mirror.begin() + i * samplesPerAngle + m_acqSettings.numberPoints * samplesPerAngle, samplesPerAngle, m_acqSettings.voltages[i].Uy);
	}
	// Apply voltages to NIDAQ board
	(*m_NIDAQ)->setAcquisitionVoltages(voltages);

	int rank_data{ 3 };
	hsize_t dims_data[3] = { 1, m_cameraSettings.roi.height, m_cameraSettings.roi.width };
	int bytesPerFrame = m_cameraSettings.roi.width * m_cameraSettings.roi.height;
	for (gsl::index i{ 0 }; i < m_acqSettings.numberPoints; i++) {

		// read images from camera
		std::vector<unsigned char> images(bytesPerFrame);

		for (gsl::index mm{ 0 }; mm < 1; mm++) {
			if (m_abort) {
				this->abortMode();
				return;
			}

			// acquire images
			int64_t pointerPos = (int64_t)bytesPerFrame * mm;
			(*m_camera)->getImageForAcquisition(&images[pointerPos], false);
		}

		// cast the vector to unsigned short
		std::vector<unsigned char>* images_ = (std::vector<unsigned char> *) &images;

		// store images
		// asynchronously write image to disk
		// the datetime has to be set here, otherwise it would be determined by the time the queue is processed
		std::string date = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
			.toString(Qt::ISODateWithMs).toStdString();
		ODTIMAGE* img = new ODTIMAGE((int)i, rank_data, dims_data, date, *images_);

		QMetaObject::invokeMethod(storage.get(), "s_enqueuePayload", Qt::AutoConnection, Q_ARG(ODTIMAGE*, img));
	}

	// Here we wait until the storage object indicate it finished to write to the file.
	QEventLoop loop;
	connect(storage.get(), SIGNAL(finished()), &loop, SLOT(quit()));
	loop.exec();

	m_status = ACQUISITION_STATUS::FINISHED;
	emit(s_acquisitionStatus(m_status));
}

void ODT::startAlignment() {
	if (!m_algnRunning) {
		bool allowed = m_acquisition->enableMode(ACQUISITION_MODE::ODT);
		if (!allowed) {
			return;
		}
		m_status = ACQUISITION_STATUS::ALIGNING;
		emit(s_acquisitionStatus(m_status));
		m_algnRunning = true;

		// configure camera exposure and gain
		CAMERA_SETTINGS settings = (*m_camera)->getSettings();
		settings.exposureTime = m_cameraSettings.exposureTime;
		settings.gain = m_cameraSettings.gain;
		QMetaObject::invokeMethod((*m_camera), "setSettings", Qt::AutoConnection, Q_ARG(CAMERA_SETTINGS, settings));

		// move to ODT configuration
		(*m_NIDAQ)->setPreset(SCAN_ODT);
		// stop querying the element positions, because querying the filter mounts block the thread quite long
		(*m_NIDAQ)->stopAnnouncingElementPosition();
		// start the timer
		if (!m_algnTimer->isActive()) {
			m_algnTimer->start(1e3 / (m_algnSettings.scanRate * m_algnSettings.numberPoints));
		}
	} else {
		m_algnRunning = false;
		if (m_algnTimer->isActive()) {
			m_algnTimer->stop();
		}
		// start querying the element positions again
		(*m_NIDAQ)->startAnnouncingElementPosition();
		m_acquisition->disableMode(ACQUISITION_MODE::ODT);
		m_status = ACQUISITION_STATUS::STOPPED;
		emit(s_acquisitionStatus(m_status));
	}
}

void ODT::centerAlignment() {
	if (!m_algnRunning && (m_status < ACQUISITION_STATUS::RUNNING)) {
		(*m_NIDAQ)->setVoltage({ 0, 0 });

		// announce mirror voltage
		emit(s_mirrorVoltageChanged({ 0, 0 }, ODT_MODE::ALGN));
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
	(*m_NIDAQ)->setVoltage(voltage);

	// announce mirror voltage
	emit(s_mirrorVoltageChanged(voltage, ODT_MODE::ALGN));
}

void ODT::calculateVoltages(ODT_MODE mode) {
	if (mode == ODT_MODE::ALGN) {
		double Ux{ 0 };
		double Uy{ 0 };
		std::vector<double> theta = simplemath::linspace<double>(0, 360, m_algnSettings.numberPoints + 1);
		theta.erase(theta.end() - 1);
		m_algnSettings.voltages.resize(theta.size());
		for (gsl::index i{ 0 }; i < theta.size(); i++) {
			Ux = m_algnSettings.radialVoltage * cos(theta[i]* M_PI / 180);
			Uy = m_algnSettings.radialVoltage * sin(theta[i]* M_PI / 180);
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

		std::vector<double> theta = simplemath::linspace<double>(2*M_PI, 0, n3);
		theta.erase(theta.begin());
		for (gsl::index i{ 0 }; i < theta.size(); i++) {

			double r = sqrt(abs(theta[i]));
			Ux = m_acqSettings.radialVoltage * r*cos(theta[i]) / sqrt(2 * M_PI);
			Uy = m_acqSettings.radialVoltage * r*sin(theta[i]) / sqrt(2 * M_PI);

			m_acqSettings.voltages.push_back({ Ux, Uy });
		}

		theta = simplemath::linspace<double>(0, 2 * M_PI, n3);
		theta.erase(theta.begin());
		theta.erase(theta.end() - 1);
		for (gsl::index i{ 0 }; i < theta.size(); i++) {

			double r = sqrt(abs(theta[i]));
			Ux = m_acqSettings.radialVoltage * -r*cos(theta[i]) / sqrt(2 * M_PI);
			Uy = m_acqSettings.radialVoltage * -r*sin(theta[i]) / sqrt(2 * M_PI);

			m_acqSettings.voltages.push_back({ Ux, Uy });
		}

		theta = simplemath::linspace<double>(0, 2 * M_PI, m_acqSettings.numberPoints - 2 * n3 + 3);
		theta = simplemath::linspace<double>(0, theta.end()[-2], m_acqSettings.numberPoints - 2 * n3 + 3);
		for (gsl::index i{ 0 }; i < theta.size(); i++) {

			Ux = -1* m_acqSettings.radialVoltage * cos(theta[i]);
			Uy = -1* m_acqSettings.radialVoltage * sin(theta[i]);

			m_acqSettings.voltages.push_back({ Ux, Uy });
		}

		m_acqSettings.numberPoints = m_acqSettings.voltages.size();

		emit(s_acqSettingsChanged(m_acqSettings));
	}
}

void ODT::abortMode() {
	// stop alignment if it is running
	if (m_algnTimer->isActive()) {
		m_algnRunning = false;
		m_algnTimer->stop();
	}
	m_acquisition->disableMode(ACQUISITION_MODE::ODT);
	m_status = ACQUISITION_STATUS::ABORTED;
	emit(s_acquisitionStatus(m_status));
}
