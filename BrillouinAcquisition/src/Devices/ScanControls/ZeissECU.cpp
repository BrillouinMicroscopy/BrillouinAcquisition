#include "stdafx.h"
#include "ZeissECU.h"

/*
 * Public definitions
 */

ZeissECU::ZeissECU() noexcept {

	m_deviceElements = {
		{ "Beam Block",	2, (int)DEVICE_ELEMENT::BEAMBLOCK, { "Close", "Open" } },
		{ "Objective",	6, (int)DEVICE_ELEMENT::OBJECTIVE },
		{ "Reflector",	5, (int)DEVICE_ELEMENT::REFLECTOR },
		{ "Tubelens",	3, (int)DEVICE_ELEMENT::TUBELENS },
		{ "Baseport",	3, (int)DEVICE_ELEMENT::BASEPORT },
		{ "Sideport",	3, (int)DEVICE_ELEMENT::SIDEPORT },
		{ "Mirror",		2, (int)DEVICE_ELEMENT::MIRROR }
	};

	m_presets = {
		{ "Brillouin",		ScanPreset::SCAN_BRILLOUIN,		{ {2}, {}, {1}, {3}, {1}, {2},  {} }	},	// Brillouin
		{ "Calibration",	ScanPreset::SCAN_CALIBRATION,	{ {2}, {}, {1}, {3}, {1}, {3},  {} }	},	// Calibration
		{ "Brightfield",	ScanPreset::SCAN_BRIGHTFIELD,	{ {2}, {}, {1}, {3}, {1}, {2}, {2} }	},	// Brightfield
		{ "Eyepiece",		ScanPreset::SCAN_EYEPIECE,		{ {2}, {}, {1}, {3}, {2}, {3}, {2} }	},	// Eyepiece
		{ "Fluo Blue",		ScanPreset::SCAN_EPIFLUOBLUE,	{ {1}, {}, {2}, {3},  {}, {2}, {1} }	},	// Fluorescence blue
		{ "Fluo Green",		ScanPreset::SCAN_EPIFLUOGREEN,	{ {1}, {}, {3}, {3},  {}, {2}, {1} }	},	// Fluorescence green
		{ "Fluo Red",		ScanPreset::SCAN_EPIFLUORED,	{ {1}, {}, {4}, {3},  {}, {2}, {1} }	},	// Fluorescence red
		{ "Laser off",		ScanPreset::SCAN_LASEROFF,		{ {1}, {},  {},  {},  {},  {},  {} }	}	// Laser off
	};

	m_elementPositions = std::vector<double>((int)DEVICE_ELEMENT::COUNT, -1);

	// Register capabilities
	registerCapability(Capabilities::TranslationStage);
	registerCapability(Capabilities::ScaleCalibration);

	/*
	 * Initialize the scale calibration with default values (determined for a 40x objective)
	 */
	auto scaleCalibration = ScaleCalibrationData{};
	scaleCalibration.micrometerToPixX = { -7.95, 9.05 };
	scaleCalibration.micrometerToPixY = { 9.45, 8.65 };

	try {
		ScaleCalibrationHelper::initializeCalibrationFromMicrometer(&scaleCalibration);

		setScaleCalibration(scaleCalibration);
	} catch (std::exception& e) {
	}
}

ZeissECU::~ZeissECU() {
	disconnectDevice();
	if (m_positionTimer) {
		m_positionTimer->stop();
		m_positionTimer->deleteLater();
	}
	if (m_elementPositionTimer) {
		m_elementPositionTimer->stop();
		m_elementPositionTimer->deleteLater();
	}
	if (m_focus) {
		m_focus->deleteLater();
	}
	if (m_mcu) {
		m_mcu->deleteLater();
	}
	if (m_stand) {
		m_stand->deleteLater();
	}
	if (m_comObject) {
		m_comObject->deleteLater();
	}
}

void ZeissECU::setPosition(POINT2 position) {
	// We have to subtract the position of the scanner to get the position of the stage.
	auto positionStage = position - m_positionScanner;
	if (abs(m_positionStage.x - positionStage.x) > 1e-6) {
		m_positionStage.x = positionStage.x;
		m_mcu->setX(m_positionStage.x);
	}
	if (abs(m_positionStage.y - positionStage.y) > 1e-6) {
		m_positionStage.y = positionStage.y;
		m_mcu->setY(m_positionStage.y);
	}
	calculateCurrentPositionBounds(POINT3{ position.x, position.y, m_positionFocus });
	announcePositions();
}

void ZeissECU::setPosition(POINT3 position) {
	// Only set position if it has changed
	if (abs(m_positionFocus - position.z) > 1e-6) {
		m_positionFocus = position.z;
		m_focus->setZ(m_positionFocus);
	}

	setPosition(POINT2{ position.x, position.y });
}

POINT3 ZeissECU::getPosition(PositionType positionType) {
	// Update the positions from the hardware
	m_positionStage.x = m_mcu->getX();
	m_positionStage.y = m_mcu->getY();

	m_positionFocus = m_focus->getZ();

	// Return the current position
	return ScanControl::getPosition(positionType);
}

void ZeissECU::setDevice(com* device) {
	if (m_comObject) {
		m_comObject->deleteLater();
		m_comObject = nullptr;
	}
	m_comObject = device;
	m_focus->setDevice(device);
	m_mcu->setDevice(device);
	m_stand->setDevice(device);
}

/*
 * Public slots
 */

void ZeissECU::init() {
	m_comObject = new com();

	m_focus = new Focus(m_comObject);
	m_mcu = new MCU(m_comObject);
	m_stand = new Stand(m_comObject);

	auto connection = QWidget::connect(
		m_comObject,
		&com::errorOccurred,
		this,
		&ZeissECU::errorHandler
	);

	m_positionTimer = new QTimer();
	connection = QWidget::connect(
		m_positionTimer,
		&QTimer::timeout,
		this,
		&ZeissECU::announcePosition
	);

	m_elementPositionTimer = new QTimer();
	connection = QWidget::connect(
		m_elementPositionTimer,
		&QTimer::timeout,
		this,
		&ZeissECU::getElements
	);
	calculateHomePositionBounds();
}

void ZeissECU::connectDevice() {
	if (!m_isConnected) {
		try {
			m_comObject->setPortName("COM1");
			if (!m_comObject->setBaudRate(QSerialPort::Baud9600)) {
				throw QString("Could not set BaudRate.");
			}
			if (!m_comObject->setFlowControl(QSerialPort::HardwareControl)) {
				throw QString("Could not set FlowControl.");
			}
			if (!m_comObject->setDataBits(QSerialPort::Data8)) {
				throw QString("Could not set DataBits.");
			}
			if (!m_comObject->setParity(QSerialPort::NoParity)) {
				throw QString("Could not set Parity.");
			}
			if (!m_comObject->setStopBits(QSerialPort::OneStop)) {
				throw QString("Could not set StopBits.");
			}
			m_isConnected = m_comObject->open(QIODevice::ReadWrite);
			if (!m_isConnected) {
				throw QString("Could not open the serial port.");
			}
			m_comObject->clear();

			auto baudRate = m_comObject->baudRate();
			auto dataBits = m_comObject->dataBits();
			auto flowControl = m_comObject->flowControl();
			auto parity = m_comObject->parity();
			auto stopBits = m_comObject->stopBits();

			Thorlabs_FF::FF_Open(m_serialNo_FF2);
			Thorlabs_FF::FF_StartPolling(m_serialNo_FF2, 200);

			// check if connected to compatible device
			auto focus = m_focus->checkCompatibility();
			auto stand = m_stand->checkCompatibility();
			auto mcu = m_mcu->checkCompatibility();

			m_isCompatible = focus && stand && mcu;

			if (m_isConnected && m_isCompatible) {
				setPreset(ScanPreset::SCAN_BRILLOUIN);
				getElements();
				m_homePosition = getPosition();
				startAnnouncingPosition();
				startAnnouncingElementPosition();
				calculateHomePositionBounds();
				calculateCurrentPositionBounds();
			} else {
				m_isConnected = false;
			}

		} catch (QString& e) {
			// todo
		}
	}
	emit(connectedDevice(m_isConnected && m_isCompatible));
}

void ZeissECU::disconnectDevice() {
	if (m_comObject && m_isConnected) {
		stopAnnouncingPosition();
		stopAnnouncingElementPosition();
		m_comObject->close();
		Thorlabs_FF::FF_Close(m_serialNo_FF2);
		Thorlabs_FF::FF_StopPolling(m_serialNo_FF2);
		m_isConnected = false;
		m_isCompatible = false;
	}
	emit(connectedDevice(m_isConnected && m_isCompatible));
}

void ZeissECU::setElement(DeviceElement element, double position) {
	switch ((DEVICE_ELEMENT)element.index) {
	case DEVICE_ELEMENT::BEAMBLOCK:
		setBeamBlock((int)position);
		break;
	case DEVICE_ELEMENT::REFLECTOR:
		m_stand->setReflector((int)position, true);
		break;
	case DEVICE_ELEMENT::OBJECTIVE:
		m_stand->setObjective((int)position, true);
		break;
	case DEVICE_ELEMENT::TUBELENS:
		m_stand->setTubelens((int)position, true);
		break;
	case DEVICE_ELEMENT::BASEPORT:
		m_stand->setBaseport((int)position, true);
		break;
	case DEVICE_ELEMENT::SIDEPORT:
		m_stand->setSideport((int)position, true);
		break;
	case DEVICE_ELEMENT::MIRROR:
		m_stand->setMirror((int)position, true);
		break;
	default:
		break;
	}
	m_elementPositions[element.index] = position;
	checkPresets();
	emit(elementPositionChanged(element, position));
}

int ZeissECU::getElement(DeviceElement element) {
	return -1;
}

void ZeissECU::getElements() {
	m_elementPositionsTmp = m_elementPositions;
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::BEAMBLOCK] = getBeamBlock();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::REFLECTOR] = m_stand->getReflector();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::OBJECTIVE] = m_stand->getObjective();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::TUBELENS] = m_stand->getTubelens();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::BASEPORT] = m_stand->getBaseport();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::SIDEPORT] = m_stand->getSideport();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::MIRROR] = m_stand->getMirror();
	// We only emit changed positions
	if (m_elementPositionsTmp != m_elementPositions) {
		m_elementPositions = m_elementPositionsTmp;
		checkPresets();
		emit(elementPositionsChanged(m_elementPositions));
	}
}

/*
 * Private definitions
 */

void ZeissECU::setBeamBlock(int position) {
	Thorlabs_FF::FF_MoveToPosition(m_serialNo_FF2, (Thorlabs_FF::FF_Positions)position);
	auto i{ 0 };
	while (getBeamBlock() != position && i++ < 10) {
		Sleep(100);
	}
}

int ZeissECU::getBeamBlock() {
	return Thorlabs_FF::FF_GetPosition(m_serialNo_FF2);
}

void ZeissECU::errorHandler(QSerialPort::SerialPortError error) {}

/*
 * Functions of the parent class for all elements
 */

/*
 * Public definitions
 */

void Element::setDevice(com* device) {
	m_comObject = device;
}

bool Element::checkCompatibility() {
	auto version = requestVersion();
	return std::find(m_versions.begin(), m_versions.end(), version) != m_versions.end();
}

/*
 * Protected definitions
 */

std::string Element::receive(std::string request) {
	auto answer = m_comObject->receive(m_prefix + "P" + request);
	return helper::parse(answer, m_prefix);
}

void Element::send(std::string message) {
	m_comObject->send(m_prefix + "P" + message);
}

void Element::clear() {
	m_comObject->clear();
}

std::string Element::requestVersion() {
	auto answer = m_comObject->receive(m_prefix + "P" + "Tv");
	return helper::parse(answer, m_prefix);
}


/*
 * Functions regarding the stand of the microscope
 */

/*
 * Public definitions
 */

void Stand::setReflector(int position, bool block) {
	if (position > 0 && position < 6) {
		setElementPosition("1", position);
	}
	blockUntilPositionReached(block, "1");
}

int Stand::getReflector() {
	return getElementPosition("1");
}

void Stand::setObjective(int position, bool block) {
	if (position > 0 && position < 7) {
		setElementPosition("2", position);
	}
	blockUntilPositionReached(block, "2");
}

int Stand::getObjective() {
	return getElementPosition("2");
}

void Stand::setTubelens(int position, bool block) {
	if (position > 0 && position < 4) {
		setElementPosition("36", position);
	}
	blockUntilPositionReached(block, "36");
}

int Stand::getTubelens() {
	return getElementPosition("36");
}

void Stand::setBaseport(int position, bool block) {
	if (position > 0 && position < 4) {
		setElementPosition("38", position);
		blockUntilPositionReached(block, "38");
	}
}

int Stand::getBaseport() {
	return getElementPosition("38");
}

void Stand::setSideport(int position, bool block) {
	if (position > 0 && position < 4) {
		setElementPosition("39", position);
	}
	blockUntilPositionReached(block, "39");
}

int Stand::getSideport() {
	return getElementPosition("39");
}

void Stand::setMirror(int position, bool block) {
	if (position > 0 && position < 3) {
		setElementPosition("51", position);
	}
	blockUntilPositionReached(block, "51");
}

int Stand::getMirror() {
	return getElementPosition("51");
}

/*
 * Private definitions
 */

void Stand::setElementPosition(std::string device, int position) {
	send("CR" + device + "," + std::to_string(position));
}

int Stand::getElementPosition(std::string device) {
	std::string answer = receive("Cr" + device + ",1");
	if (answer.empty()) {
		return -1;
	}
	else {
		return std::stoi(answer);
	}
}

void Stand::blockUntilPositionReached(bool block, std::string elementNr) {
	// don't return until the position or the timeout is reached
	if (block) {
		auto count{ 0 };
		auto pos = getElementPosition(elementNr);
		// wait for one second max
		while (!pos && count < 100) {
			Sleep(10);
			pos = getElementPosition(elementNr);
			count++;
		}
		//TODO: Emit an error when count==100 (timeout reached)
	}
}

/*
 * Functions regarding the objective focus
 */

/*
 * Public definitions
 */

void Focus::setZ(double position) {
	position = round(position / m_umperinc);
	auto inc = positive_modulo(position, m_rangeFocus);

	auto pos = helper::dec2hex(inc, 6);
	send("ZD" + pos);
	clear();
}

double Focus::getZ() {
	auto position = "0x" + receive("Zp");
	auto pos = helper::hex2dec(position);
	// The actual travel range of the focus is significantly smaller than the theoretically possible maximum increment value (FFFFFF or 16777215).
	// When the microscope starts, it sets it home position to (0, 0, 0). Values in the negative range are then adressed as (16777215 - positionInInc).
	// Hence, we consider all values > 16777215/2 to actually be negative and wrap them accordingly (similar to what positive_modulo(...,...) for the setPosition() functions does).
	if (pos > m_rangeFocus / 2) {
		pos -= m_rangeFocus;
	}
	return pos * m_umperinc;
}

void Focus::setVelocityZ(double velocity) {
	auto vel = helper::dec2hex(velocity, 6);
	send("ZG" + vel);
}

void Focus::scanUp() {
	send("ZS+");
}

void Focus::scanDown() {
	send("ZS-");
}

void Focus::scanStop() {
	send("ZSS");
}

int Focus::getScanStatus() {
	auto status = receive("Zt");
	return std::stoi(status);
}

int Focus::getStatusKey() {
	auto status = receive("Zw");
	return std::stoi(status);
}

void Focus::move2Load() {
	send("ZW0");
}

void Focus::move2Work() {
	send("ZW1");
}


/*
 * Functions regarding the stage of the microscope
 */

/*
 * Public definitions
 */

double MCU::getX() {
	return getPosition("X");
}

void MCU::setX(double position) {
	setPosition("X", position);
}

double MCU::getY() {
	return getPosition("Y");
}

void MCU::setY(double position) {
	setPosition("Y", position);
}

void MCU::setVelocityX(int velocity) {
	setVelocity("X", velocity);
}

void MCU::setVelocityY(int velocity) {
	setVelocity("Y", velocity);
}

void MCU::stopX() {
	send("XS");
}

void MCU::stopY() {
	send("YS");
}

/*
 * Private definitions
 */

void MCU::setPosition(std::string axis, double position) {
	position = round(position / m_umperinc);
	auto inc = positive_modulo(position, m_rangeFocus);

	auto pos = helper::dec2hex(inc, 6);
	send(axis + "T" + pos);
}

double MCU::getPosition(std::string axis) {
	auto position = receive(axis + "p");
	auto pos = helper::hex2dec(position);
	// The actual travel range of the stage is significantly smaller than the theoretically possible maximum increment value (FFFFFF or 16777215).
	// When the microscope starts, it sets it home position to (0, 0, 0). Values in the negative range are then adressed as (16777215 - positionInInc).
	// Hence, we consider all values > 16777215/2 to actually be negative and wrap them accordingly (similar to what positive_modulo(...,...) for the setPosition() functions does).
	if (pos > m_rangeFocus / 2) {
		pos -= m_rangeFocus;
	}
	return pos * m_umperinc;
}

void MCU::setVelocity(std::string axis, int velocity) {
	auto vel = helper::dec2hex(velocity, 6);
	send(axis + "V" + vel);
}