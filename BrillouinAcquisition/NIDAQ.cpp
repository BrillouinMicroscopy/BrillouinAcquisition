#include "stdafx.h"
#include "NIDAQ.h"

VOLTAGE2 NIDAQ::positionToVoltage(POINT2 position) {

	position = position - m_calibration.translation;

	double x_rot = position.x * cos(m_calibration.rho) + position.y * sin(m_calibration.rho);
	double y_rot = position.y * cos(m_calibration.rho) - position.x * sin(m_calibration.rho);

	double R_old = sqrt(pow(position.x, 2) + pow(position.y, 2));

	VOLTAGE2 voltage{ 0, 0 };
	if (R_old == 0) {
		voltage = { x_rot, y_rot };
		return voltage;
	}

	// solve for R_new
	
	COEFFICIANTS5 coef = m_calibration.coef;
	coef.e = -1 * R_old;
	std::vector<std::complex<double>> solutions = simplemath::solveQuartic(coef);

	// select real, positive solutions
	std::vector<double> selected;
	for (gsl::index i = 0; i < solutions.size(); i++) {
		if (abs((solutions[i]).imag()) < 1e-16 && solutions[i].real() > 0) {
			selected.push_back(solutions[i].real());
		}
	};

	double R_new{ 0 };
	if (selected.empty()) {
		R_new = NAN;
	} else if (selected.size() == 1) {
		R_new = selected[0];
	} else {
		R_new = simplemath::min(selected);
	}

	voltage = { x_rot / R_old * R_new, y_rot / R_old * R_new };
	return voltage;
}

POINT2 NIDAQ::voltageToPosition(VOLTAGE2 voltage) {

	double R_old = sqrt(pow(voltage.Ux, 2) + pow(voltage.Uy, 2));
	double R_new = m_calibration.coef.d * R_old + m_calibration.coef.c * pow(R_old, 2) +
				   m_calibration.coef.b * pow(R_old, 3) + m_calibration.coef.a * pow(R_old, 4);

	double Ux_rot = voltage.Ux * cos(m_calibration.rho) - voltage.Uy * sin(m_calibration.rho);
	double Uy_rot = voltage.Ux * sin(m_calibration.rho) + voltage.Uy * cos(m_calibration.rho);

	POINT2 position{ 0, 0 };
	if (R_old == 0) {
		position = { Ux_rot, Uy_rot };
	} else {
		position = { Ux_rot / R_old * R_new, Uy_rot / R_old * R_new };
	}
	
	position = position + m_calibration.translation;

	return position;
}

NIDAQ::NIDAQ() noexcept {
	m_availablePresets = {};
	m_availableElements = {};
}

NIDAQ::~NIDAQ() {
	disconnectDevice();
}

bool NIDAQ::connectDevice() {
	if (!m_isConnected) {
		// Connect to DAQ
		DAQmxCreateTask("", &taskHandle);
		// Configure analog output channels
		DAQmxCreateAOVoltageChan(taskHandle, "Dev1/ao0:1", "", -1.0, 1.0, DAQmx_Val_Volts, "");
		// Start DAQ task
		DAQmxStartTask(taskHandle);
		m_isConnected = true;
		m_isCompatible = true;
	}
	emit(connectedDevice(m_isConnected && m_isCompatible));
	return m_isConnected && m_isCompatible;
}

bool NIDAQ::disconnectDevice() {
	if (m_isConnected) {
		// Stop and clear DAQ task
		DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle);
		m_isConnected = false;
		m_isCompatible = false;
	}
	emit(connectedDevice(m_isConnected && m_isCompatible));
	return m_isConnected && m_isCompatible;
}

void NIDAQ::init() {
}

void NIDAQ::setElement(ScanControl::DEVICE_ELEMENT element, int position) {
}

void NIDAQ::setElements(ScanControl::SCAN_PRESET preset) {
}

void NIDAQ::getElements() {
}

void NIDAQ::setPosition(POINT3 position) {
	m_position = position;
	m_voltages = positionToVoltage(POINT2{ m_position.x, m_position.y });
	float64 data[2] = { m_voltages.Ux, m_voltages.Uy };
	DAQmxWriteAnalogF64(taskHandle, 1, true, 10.0, DAQmx_Val_GroupByChannel, data, NULL, NULL);
	// set z-position once implemented
}

void NIDAQ::movePosition(POINT3 distance) {
}

void NIDAQ::setPositionRelativeX(double positionX) {
	m_position.x = positionX;
	setPosition(m_position);
}

void NIDAQ::setPositionRelativeY(double positionY) {
	m_position.y = positionY;
	setPosition(m_position);
}

void NIDAQ::setPositionRelativeZ(double positionZ) {
	m_position.z = positionZ;
	setPosition(m_position);
}

void NIDAQ::loadCalibration(std::string filepath) {
	// TODO: read real values from the calibration file
	m_calibration.translation.x = 0;
	m_calibration.translation.y = 0;
	m_calibration.rho = 0;
	m_calibration.coef.a = 0;
	m_calibration.coef.b = 0;
	m_calibration.coef.c = 0;
	m_calibration.coef.d = 0;
	m_calibration.range.xMin = 0;
	m_calibration.range.xMax = 0;
	m_calibration.range.yMin = 0;
	m_calibration.range.yMax = 0;
	m_calibration.valid = true;
}

POINT3 NIDAQ::getPosition() {
	return m_position;
}