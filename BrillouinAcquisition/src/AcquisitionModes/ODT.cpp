#include "stdafx.h"
#include "..\simplemath.h"
#include "ODT.h"

ODT::ODT(QObject *parent, Acquisition *acquisition, PointGrey **pointGrey, NIDAQ **nidaq)
	: AcquisitionMode(parent, acquisition), m_pointGrey(pointGrey), m_NIDAQ(nidaq) {
	m_cameraOptions = (*m_pointGrey)->getOptions();
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

void ODT::init() {
	m_algnTimer = new QTimer();
	QMetaObject::Connection connection = QWidget::connect(m_algnTimer, SIGNAL(timeout()), this, SLOT(nextAlgnPosition()));
}

void ODT::initialize() {
	calculateVoltages(ODT_MODE::ALGN);
	calculateVoltages(ODT_MODE::ACQ);
}

void ODT::startRepetitions() {
	bool allowed = m_acquisition->startMode(ACQUISITION_MODE::ODT);
	if (!allowed) {
		return;
	}

	// stop alignment if it is running
	if (m_algnTimer->isActive()) {
		m_algnTimer->stop();
	}

	// reset abort flag
	m_abort = false;

	// configure camera for measurement
	configureCamera(ODT_MODE::ACQ);

	// read back the applied settings
	m_acqSettings.camera = (*m_pointGrey)->getSettings();

	(*m_pointGrey)->startAcquisition();

	m_acquisition->newRepetition(ACQUISITION_MODE::ODT);

	// start repetition
	acquire(m_acquisition->m_storage);

	// configure camera for preview
	(*m_pointGrey)->stopAcquisition();
	configureCamera(ODT_MODE::ALGN);

	m_acquisition->stopMode(ACQUISITION_MODE::ODT);
}

void ODT::acquire(std::unique_ptr <StorageWrapper> & storage) {

	VOLTAGE2 voltage;

	/*
	 * Set the mirror voltage and get the camera images
	 */
	int rank_data{ 3 };
	hsize_t dims_data[3] = { 1, m_acqSettings.camera.roi.height, m_acqSettings.camera.roi.width };
	int bytesPerFrame = m_acqSettings.camera.roi.width * m_acqSettings.camera.roi.height;
	for (gsl::index i{ 0 }; i < m_acqSettings.numberPoints; i++) {
		if (m_abort) {
			this->abortMode();
			return;
		}
		voltage = m_acqSettings.voltages[i];
		// set new voltage to galvo mirrors
		(*m_NIDAQ)->setVoltage(voltage);

		// announce mirror voltage
		emit(s_mirrorVoltageChanged(voltage, ODT_MODE::ACQ));
		
		// wait appropriate time
		Sleep(2);

		// trigger image acquisition
		(*m_NIDAQ)->triggerCamera();
	}


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
			(*m_pointGrey)->getImageForAcquisition(&images[pointerPos]);
		}

		// cast the vector to unsigned short
		std::vector<unsigned char> *images_ = (std::vector<unsigned char> *) &images;

		// store images
		// asynchronously write image to disk
		// the datetime has to be set here, otherwise it would be determined by the time the queue is processed
		std::string date = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
			.toString(Qt::ISODateWithMs).toStdString();
		ODTIMAGE *img = new ODTIMAGE((int)i, rank_data, dims_data, date, *images_);

		QMetaObject::invokeMethod(storage.get(), "s_enqueuePayload", Qt::AutoConnection, Q_ARG(ODTIMAGE*, img));
	}
}

void ODT::startAlignment() {
	if (!m_algnRunning) {
		bool allowed = m_acquisition->startMode(ACQUISITION_MODE::ODT);
		if (!allowed) {
			return;
		}
		m_algnRunning = true;
		// start the timer
		if (!m_algnTimer->isActive()) {
			m_algnTimer->start(1e3 / (m_algnSettings.scanRate * m_algnSettings.numberPoints));
		}
	} else {
		m_algnRunning = false;
		if (m_algnTimer->isActive()) {
			m_algnTimer->stop();
		}
		m_acquisition->stopMode(ACQUISITION_MODE::ODT);
	}
	emit(s_algnRunning(m_algnRunning));
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

void ODT::configureCamera(ODT_MODE mode) {
	CAMERA_SETTINGS settings;
	switch (mode) {
		case ODT_MODE::ACQ:
			settings.exposureTime = 0.005; // [s]
			settings.roi.left = 128;
			settings.roi.top = 0;
			settings.roi.width = 1024;
			settings.roi.height = 1024;
			settings.readout.pixelEncoding = L"Raw8";
			settings.readout.triggerMode = L"External";
			settings.readout.cycleMode = L"Continuous";
			settings.frameCount = m_acqSettings.numberPoints;
			break;
		case ODT_MODE::ALGN:
			settings.exposureTime = 0.005; // [s]
			settings.roi.left = 0;
			settings.roi.top = 0;
			settings.roi.width = m_cameraOptions.ROIWidthLimits[1];
			settings.roi.height = m_cameraOptions.ROIHeightLimits[1];
			settings.readout.pixelEncoding = L"Raw8";
			settings.readout.triggerMode = L"Software";
			settings.readout.cycleMode = L"Fixed";
			settings.frameCount = 10;
			break;
		default:
			return;
	}
	(*m_pointGrey)->setSettings(settings);
}

void ODT::abortMode() {
	// stop alignment if it is running
	if (m_algnTimer->isActive()) {
		m_algnTimer->stop();
	}
	m_acquisition->stopMode(ACQUISITION_MODE::ODT);
}
