#include "stdafx.h"
#include "simplemath.h"
#include "ODT.h"

ODT::ODT(QObject *parent, PointGrey **pointGrey, NIDAQ **nidaq)
	: QObject(parent), m_pointGrey(pointGrey), m_NIDAQ(nidaq) {
}

ODT::~ODT() {
	m_acqRunning = false;
	m_algnRunning = false;
}

bool ODT::isAcqRunning() {
	return m_acqRunning;
}

bool ODT::isAlgnRunning() {
	return m_algnRunning;
}

void ODT::abortAcquisition() {
	m_abortAcquisition = true;
}

void ODT::setAlgnSettings(ODT_SETTINGS settings) {
	m_algnSettings = settings;
	calculateVoltages(ALGN);
}

void ODT::setAcqSettings(ODT_SETTINGS settings) {
	m_acqSettings = settings;
	calculateVoltages(ACQ);
}

void ODT::setSettings(ODT_MODES mode, ODT_SETTING settingType, double value) {
	ODT_SETTINGS *settings;
	if (mode == ACQ) {
		settings = &m_acqSettings;
	} else if (mode == ALGN) {
		settings = &m_algnSettings;
	} else {
		return;
	}

	switch (settingType) {
		case VOLTAGE:
			settings->radialVoltage = value;
			calculateVoltages(mode);
			break;
		case NRPOINTS:
			settings->numberPoints = value;
			calculateVoltages(mode);
			if (mode == ALGN && m_algnRunning) {
				m_algnTimer->start(1e3 / (m_algnSettings.scanRate * m_algnSettings.numberPoints));
			}
			break;
		case SCANRATE:
			settings->scanRate = value;
			if (mode == ALGN && m_algnRunning) {
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
	calculateVoltages(ALGN);
	calculateVoltages(ACQ);
}

void ODT::startAcquisition() {
	if (!m_acqRunning) {
		// start acquisition
		m_acqRunning = true;
		m_abortAcquisition = false;
		emit(s_acqRunning(m_acqRunning));
		acquisition();
	} else {
		// abort acquisition
		m_acqRunning = false;
		emit(s_acqRunning(m_acqRunning));
	}
}

void ODT::acquisition() {
	if (m_acqRunning) {
		VOLTAGE2 voltage;
		for (gsl::index i{ 0 }; i < m_acqSettings.numberPoints; i++) {
			if (m_abortAcquisition) {
				break;
			}
			voltage = m_acqSettings.voltages[i];
			// set new voltage to galvo mirrors

			// announce mirror voltage
			emit(s_mirrorVoltageChanged(voltage, ACQ));

			// wait appropriate time
			Sleep(10);

			// trigger image acquisition
		}

		// read images from camera

		// store images
		m_acqRunning = false;
	}
	emit(s_acqRunning(m_algnRunning));
}

void ODT::startAlignment() {
	if (!m_algnRunning) {
		m_algnRunning = true;
		if (!m_algnTimer->isActive()) {
			m_algnTimer->start(1e3 / (m_algnSettings.scanRate * m_algnSettings.numberPoints));
		}
	} else {
		m_algnRunning = false;
		if (m_algnTimer->isActive()) {
			m_algnTimer->stop();
		}
	}
	emit(s_algnRunning(m_algnRunning));
}

void ODT::nextAlgnPosition() {
	if (++m_algnPositionIndex >= m_algnSettings.numberPoints) {
		m_algnPositionIndex = 0;
	}
	VOLTAGE2 voltage = m_algnSettings.voltages[m_algnPositionIndex];
	// set new voltage to galvo mirrors
	(*m_NIDAQ)->setVoltage(voltage);

	// announce mirror voltage
	emit(s_mirrorVoltageChanged(voltage, ALGN));
}

void ODT::calculateVoltages(ODT_MODES mode) {
	if (mode == ALGN) {
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
	if (mode == ACQ) {
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
