#include "stdafx.h"
#include "ZeissECU.h"
#include <iomanip>
#include <sstream>
#include <regex>

std::string helper::dec2hex(int dec, int digits = 6) {
	std::stringstream stream;
	// make letters in hex string uppercase -> std:uppercase
	// set the fill character --> std::setfill
	// set the number of characters --> std::setw
	stream << std::uppercase << std::setfill('0') << std::setw(digits) << std::hex << dec;
	return stream.str();
}

int helper::hex2dec(std::string s) {
	if (s.size() < 6) {
		return NAN;
	} else {
		return std::stoul(s, nullptr, 16);
	}
}

std::string helper::parse(std::string answer, std::string prefix) {
	std::string pattern = "([a-zA-Z\\d\\s_.]*)\r";
	pattern = "P" + prefix + pattern;
	std::regex pieces_regex(pattern);
	std::smatch pieces_match;
	std::regex_match(answer, pieces_match, pieces_regex);
	if (pieces_match.size() == 0) {
		return "";
	}
	else {
		return pieces_match[1];
	}
}

ZeissECU::ZeissECU() noexcept {
	m_availablePresets = { 0, 1, 2, 3 };
	m_presets = {
		{ 1, 1, 3, 1, 2, 2 },	// Brightfield
		{ 1, 1, 3, 1, 3, 2 },	// Calibration
		{ 1, 1, 3, 1, 2, 1 },	// Brillouin
		{ 1, 1, 3, 2, 3, 2 },	// Eyepiece
	};

	// bounds of the stage
	m_absoluteBounds = {
		-150000,	// [µm] minimal x-value
		 150000,	// [µm] maximal x-value
		-150000,	// [µm] minimal y-value
		 150000,	// [µm] maximal y-value
		-150000,	// [µm] minimal z-value
		 150000		// [µm] maximal z-value
	};

	m_deviceElements = {
		{ "Reflector",	5, (int)DEVICE_ELEMENT::REFLECTOR },
		{ "Objective",	6, (int)DEVICE_ELEMENT::OBJECTIVE },
		{ "Tubelens",	3, (int)DEVICE_ELEMENT::TUBELENS },
		{ "Baseport",	3, (int)DEVICE_ELEMENT::BASEPORT },
		{ "Sideport",	3, (int)DEVICE_ELEMENT::SIDEPORT },
		{ "Mirror",		2, (int)DEVICE_ELEMENT::MIRROR }
	};
}

ZeissECU::~ZeissECU() {
	positionTimer->stop();
	elementPositionTimer->stop();
	disconnectDevice();
	delete m_focus;
	delete m_mcu;
	delete m_stand;
	delete m_comObject;
}

void ZeissECU::init() {
	m_comObject = new com();

	m_focus = new Focus(m_comObject);
	m_mcu = new MCU(m_comObject);
	m_stand = new Stand(m_comObject);

	QMetaObject::Connection connection = QWidget::connect(
		m_comObject,
		SIGNAL(errorOccurred(QSerialPort::SerialPortError)),
		this,
		SLOT(errorHandler(QSerialPort::SerialPortError))
	);

	positionTimer = new QTimer();
	connection = QWidget::connect(positionTimer, SIGNAL(timeout()), this, SLOT(announcePosition()));

	elementPositionTimer = new QTimer();
	connection = QWidget::connect(elementPositionTimer, SIGNAL(timeout()), this, SLOT(getElements()));
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

			int baudRate = m_comObject->baudRate();
			QSerialPort::DataBits dataBits = m_comObject->dataBits();
			QSerialPort::FlowControl flowControl = m_comObject->flowControl();
			QSerialPort::Parity parity = m_comObject->parity();
			QSerialPort::StopBits stopBits = m_comObject->stopBits();

			// check if connected to compatible device
			bool focus = m_focus->checkCompatibility();
			bool stand = m_stand->checkCompatibility();
			bool mcu = m_mcu->checkCompatibility();

			m_isCompatible = focus && stand && mcu;

			if (m_isConnected && m_isCompatible) {
				setElements(SCAN_PRESET::SCAN_BRIGHTFIELD);
				getElements();
				m_homePosition = getPosition();
				startAnnouncingPosition();
				startAnnouncingElementPosition();
				calculateHomePositionBounds();
				calculateCurrentPositionBounds();
			} else {
				m_isConnected = false;
			}

		} catch (QString e) {
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
		m_isConnected = false;
		m_isCompatible = false;
	}
	emit(connectedDevice(m_isConnected && m_isCompatible));
}

void ZeissECU::errorHandler(QSerialPort::SerialPortError error) {
}

void ZeissECU::setPosition(POINT3 position) {
	m_mcu->setX(position.x);
	m_mcu->setY(position.y);
	m_focus->setZ(position.z);
	calculateCurrentPositionBounds();
}

void ZeissECU::setPositionRelativeX(double positionX) {
	m_mcu->setX(positionX + m_homePosition.x);
	calculateCurrentPositionBounds();
}

void ZeissECU::setPositionRelativeY(double positionY) {
	m_mcu->setY(positionY + m_homePosition.y);
	calculateCurrentPositionBounds();
}

void ZeissECU::setPositionRelativeZ(double positionZ) {
	m_focus->setZ(positionZ + m_homePosition.z);
	calculateCurrentPositionBounds();
}

POINT3 ZeissECU::getPosition() {
	double x = m_mcu->getX();
	double y = m_mcu->getY();
	double z = m_focus->getZ();
	return POINT3{ x, y, z };
}

void ZeissECU::setDevice(com *device) {
	delete m_comObject;
	m_comObject = device;
	m_focus->setDevice(device);
	m_mcu->setDevice(device);
	m_stand->setDevice(device);
}

void ZeissECU::setElements(ScanControl::SCAN_PRESET preset) {
	m_stand->setReflector(m_presets[preset][0]);
	m_stand->setObjective(m_presets[preset][1]);
	m_stand->setTubelens(m_presets[preset][2]);
	m_stand->setBaseport(m_presets[preset][3]);
	m_stand->setSideport(m_presets[preset][4]);
	m_stand->setMirror(m_presets[preset][5]);
	emit(elementPositionsChanged(m_presets[preset]));
}

void ZeissECU::setElement(DeviceElement element, int position) {
	switch ((DEVICE_ELEMENT)element.index) {
		case DEVICE_ELEMENT::REFLECTOR:
			m_stand->setReflector(position);
			break;
		case DEVICE_ELEMENT::OBJECTIVE:
			m_stand->setObjective(position);
			break;
		case DEVICE_ELEMENT::TUBELENS:
			m_stand->setTubelens(position);
			break;
		case DEVICE_ELEMENT::BASEPORT:
			m_stand->setBaseport(position);
			break;
		case DEVICE_ELEMENT::SIDEPORT:
			m_stand->setSideport(position);
			break;
		case DEVICE_ELEMENT::MIRROR:
			m_stand->setMirror(position);
			break;
		default:
			break;
	}
	emit(elementPositionChanged(element, position));
}

void ZeissECU::getElements() {
	std::vector<int> elementPositions(m_deviceElements.deviceCount(), -1);
	elementPositions[0] = m_stand->getReflector();
	elementPositions[1] = m_stand->getObjective();
	elementPositions[2] = m_stand->getTubelens();
	elementPositions[3] = m_stand->getBaseport();
	elementPositions[4] = m_stand->getSideport();
	elementPositions[5] = m_stand->getMirror();
	emit(elementPositionsChanged(elementPositions));
};

void ZeissECU::getElement(DeviceElement element) {
}

/*
* Functions regarding the serial communication
*
*/

std::string com::receive(std::string request) {
	request = request + m_terminator;
	writeToDevice(request.c_str());

	std::string response = "";
	if (waitForBytesWritten(1000)) {
		// read response
		if (waitForReadyRead(1000)) {
			QByteArray responseData = readAll();
			while (waitForReadyRead(50))
				responseData += readAll();

			response = responseData;
		}
	}

	return response;
}

void com::send(std::string message) {
	message = message + m_terminator;

	writeToDevice(message.c_str());
	bool wasWritten = waitForBytesWritten(1000);

	if (!wasWritten) {
		int tmp = 0;
	}
}

qint64 com::writeToDevice(const char *data) {
	return write(data);
}

/*
* Functions of the parent class for all elements
*
*/

Element::~Element() {
}

std::string Element::receive(std::string request) {
	std::string answer = m_comObject->receive(m_prefix + "P" + request);
	return helper::parse(answer, m_prefix);
}

void Element::send(std::string message) {
	m_comObject->send(m_prefix + "P" + message);
}

void Element::setDevice(com *device) {
	m_comObject = device;
}

std::string Element::requestVersion() {
	std::string answer = m_comObject->receive(m_prefix + "P" + "Tv");
	return helper::parse(answer, m_prefix);
}

bool Element::checkCompatibility() {
	std::string version = requestVersion();
	return std::find(m_versions.begin(), m_versions.end(), version) != m_versions.end();
}

/*
* Functions regarding the objective focus
*
*/

double Focus::getZ() {
	std::string position = "0x" + receive("Zp");
	int pos = helper::hex2dec(position);
	// The actual travel range of the focus is significantly smaller than the theoretically possible maximum increment value (FFFFFF or 16777215).
	// When the microscope starts, it sets it home position to (0, 0, 0). Values in the negative range are then adressed as (16777215 - positionInInc).
	// Hence, we consider all values > 16777215/2 to actually be negative and wrap them accordingly (similar to what positive_modulo(...,...) for the setPosition() functions does).
	if (pos > m_rangeFocus / 2) {
		pos -= m_rangeFocus;
	}
	return pos * m_umperinc;
}

void Focus::setZ(double position) {
	position = round(position / m_umperinc);
	int inc = positive_modulo(position, m_rangeFocus);

	std::string pos = helper::dec2hex(inc, 6);
	std::string answer = receive("ZD" + pos);
}

void Focus::setVelocityZ(double velocity) {
	std::string vel = helper::dec2hex(velocity, 6);
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
	std::string status = receive("Zt");
	return std::stoi(status);
}

int Focus::getStatusKey() {
	std::string status = receive("Zw");
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
 *
 */

double MCU::getPosition(std::string axis) {
	std::string position = receive(axis + "p");
	int pos = helper::hex2dec(position);
	// The actual travel range of the stage is significantly smaller than the theoretically possible maximum increment value (FFFFFF or 16777215).
	// When the microscope starts, it sets it home position to (0, 0, 0). Values in the negative range are then adressed as (16777215 - positionInInc).
	// Hence, we consider all values > 16777215/2 to actually be negative and wrap them accordingly (similar to what positive_modulo(...,...) for the setPosition() functions does).
	if (pos > m_rangeFocus /2) {
		pos -= m_rangeFocus;
	}
	return pos * m_umperinc;
}

void MCU::setPosition(std::string axis, double position) {
	position = round(position / m_umperinc);
	int inc = positive_modulo(position, m_rangeFocus);

	std::string pos = helper::dec2hex(inc, 6);
	send(axis + "T" + pos);
}

void MCU::setVelocity(std::string axis, int velocity) {
	std::string vel = helper::dec2hex(velocity, 6);
	send(axis + "V" + vel);
}

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
 * Functions regarding the stand of the microscope
 *
 */

int Stand::getElementPosition(std::string device) {
	std::string answer = receive("Cr" + device + ",1");
	if (answer.empty()) {
		return -1;
	} else {
		return std::stoi(answer);
	}
};

void Stand::setElementPosition(std::string device, int position) {
	send("CR" + device + "," + std::to_string(position));
}

int Stand::getReflector() {
	return getElementPosition("1");
}

void Stand::setReflector(int position) {
	if (position > 0 && position < 6) {
		setElementPosition("1", position);
	}
}

int Stand::getObjective() {
	return getElementPosition("2");
}

void Stand::setObjective(int position) {
	if (position > 0 && position < 7) {
		setElementPosition("2", position);
	}
}

int Stand::getTubelens() {
	return getElementPosition("36");
}

void Stand::setTubelens(int position) {
	if (position > 0 && position < 4) {
		setElementPosition("36", position);
	}
}

int Stand::getBaseport() {
	return getElementPosition("38");
}

void Stand::setBaseport(int position) {
	if (position > 0 && position < 4) {
		setElementPosition("38", position);
	}
}

int Stand::getSideport() {
	return getElementPosition("39");
}

void Stand::setSideport(int position) {
	if (position > 0 && position < 4) {
		setElementPosition("39", position);
	}
}

int Stand::getMirror() {
	return getElementPosition("51");
}

void Stand::setMirror(int position) {
	if (position > 0 && position < 3) {
		setElementPosition("51", position);
	}
}