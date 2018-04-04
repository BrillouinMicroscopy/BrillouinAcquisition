#include "stdafx.h"
#include "scancontrol.h"
#include <iomanip>
#include <sstream>
#include <regex>

ScanControl::ScanControl() {
	m_focus = new Focus(m_comObject);
	m_mcu = new MCU(m_comObject);
	m_stand = new Stand(m_comObject);
}

ScanControl::~ScanControl() {
	disconnect();
	delete m_focus;
	delete m_mcu;
	delete m_stand;
	delete m_comObject;
}

bool ScanControl::connect() {
	if (!isConnected) {
		m_comObject->setBaudRate(QSerialPort::Baud9600);
		m_comObject->setPortName("COM1");
		m_comObject->setFlowControl(QSerialPort::HardwareControl);
		isConnected = m_comObject->open(QIODevice::ReadWrite);
	}
	emit(microscopeConnected(isConnected));
	return isConnected;
}

bool ScanControl::disconnect() {
	if (m_comObject && isConnected) {
		m_comObject->close();
		isConnected = FALSE;
	}
	emit(microscopeConnected(isConnected));
	return isConnected;
}

bool ScanControl::getConnectionStatus() {
	return isConnected;
}

void ScanControl::setPosition(std::vector<double> position) {
	m_mcu->setX(position[0]);
	m_mcu->setY(position[1]);
	m_focus->setZ(position[2]);
}

void ScanControl::setPositionRelative(std::vector<double> distance) {
	std::vector<double> position = getPosition();
	m_mcu->setX(position[0] + distance[0]);
	m_mcu->setY(position[1] + distance[1]);
	m_focus->setZ(position[2] + distance[2]);
}

std::vector<double> ScanControl::getPosition() {
	double x = m_mcu->getX();
	double y = m_mcu->getY();
	double z = m_focus->getZ();
	return std::vector<double> {x, y, z};
}


/*
* Functions regarding the serial communication
*
*/

std::string com::receive(std::string request) {
	request = request + "\r";
	write(request.c_str());

	int toWrite = bytesToWrite();
	waitForBytesWritten();
	bool wasWritten = flush();

	waitForReadyRead();

	char buf[1024];
	std::string answer;

	int bytesRead = readLineDataCR(buf, 1024);
	if (bytesRead > -1)
		 answer = buf;
	else
		answer = "";

	return answer;
}

qint64 com::readLineDataCR(char *data, qint64 maxSize) {
	qint64 readSoFar = 0;
	char c;
	int lastReadReturn = 0;

	while (readSoFar < maxSize && (lastReadReturn = read(&c, 1)) == 1) {
		*data++ = c;
		++readSoFar;
		if (c == '\r')
			break;
	}

	if (lastReadReturn != 1 && readSoFar == 0)
		return isSequential() ? lastReadReturn : -1;
	return readSoFar;
}

void com::send(std::string message) {
	message = message + "\r";
	write(message.c_str());
}


/*
* Functions of the parent class for all elements
*
*/

Element::~Element() {
}

std::string Element::parse(std::string answer) {
	std::string pattern = "([a-zA-Z\\d]*)\r";
	pattern = "P" + m_prefix + pattern;
	std::regex pieces_regex(pattern);
	std::smatch pieces_match;
	std::regex_match(answer, pieces_match, pieces_regex);
	if (pieces_match.size() == 0) {
		return "";
	} else {
		return pieces_match[1];
	}
}

std::string Element::receive(std::string request) {
	std::string answer = m_comObject->receive(m_prefix + "P" + request);
	return parse(answer);
}

void Element::send(std::string message) {
	m_comObject->send(m_prefix + "P" + message);
}

std::string Element::dec2hex(int dec, int digits = 6) {
	std::stringstream stream;
	stream << std::hex << dec;
	std::string hex = stream.str();
	// pad hex string with leading zeros if necessary
	int len = hex.length();
	if (len < digits) {
		hex = std::string(digits - len, '0') + hex;
	}
	return hex;
}

int Element::hex2dec(std::string s) {
	return std::stoul(s, nullptr, 16);
}


/*
* Functions regarding the objective focus
*
*/

double Focus::getZ() {
	std::string s = "0x" + receive("Zp");
	return m_umperinc * hex2dec(s);
}

void Focus::setZ(double position) {
	position = round(position / m_umperinc);
	int inc = static_cast<int>(position);
	inc %= m_rangeFocus;
	std::string pos = dec2hex(inc, 6);
	receive("ZD" + pos);
}

void Focus::setVelocityZ(double velocity) {
	std::string vel = dec2hex(velocity, 6);
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
	int pos = hex2dec(position);
	return pos * m_umperinc;
}

void MCU::setPosition(std::string axis, double position) {
	position = round(position / m_umperinc);
	int inc = static_cast<int>(position);
	inc %= m_rangeFocus;
	std::string pos = dec2hex(inc, 6);
	send(axis + "T" + pos);
}

void MCU::setVelocity(std::string axis, int velocity) {
	std::string vel = dec2hex(velocity, 6);
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

void Stand::getElementPositions() {
	std::vector<int> elementPositions(6,0);
	elementPositions[0] = getReflector();
	elementPositions[1] = getObjective();
	elementPositions[2] = getTubelens();
	elementPositions[3] = getBaseport();
	elementPositions[4] = getSideport();
	elementPositions[5] = getMirror();
	emit(elementPositionsChanged(elementPositions));
};

int Stand::getReflector() {
	std::string answer = receive("Cr1,1");
	return std::stoi(answer);
}

void Stand::setReflector(int position) {
	if (position > 0 && position < 6) {
		send("CR1," + std::to_string(position));
	}
	emit(elementPositionsChanged(0, position));
}

int Stand::getObjective() {
	std::string answer = receive("Cr2,1");
	return std::stoi(answer);
}

void Stand::setObjective(int position) {
	if (position > 0 && position < 7) {
		send("CR2," + std::to_string(position));
	}
	emit(elementPositionsChanged(1, position));
}

int Stand::getTubelens() {
	std::string answer = receive("Cr36,1");
	return std::stoi(answer);
}

void Stand::setTubelens(int position) {
	if (position > 0 && position < 4) {
		send("CR36," + std::to_string(position));
	}
	emit(elementPositionsChanged(2, position));
}

int Stand::getBaseport() {
	std::string answer = receive("Cr38,1");
	return std::stoi(answer);
}

void Stand::setBaseport(int position) {
	if (position > 0 && position < 4) {
		send("CR38," + std::to_string(position));
	}
	emit(elementPositionsChanged(3, position));
}

int Stand::getSideport() {
	std::string answer = receive("Cr39,1");
	return std::stoi(answer);
}

void Stand::setSideport(int position) {
	if (position > 0 && position < 4) {
		send("CR39," + std::to_string(position));
	}
	emit(elementPositionsChanged(4, position));
}

int Stand::getMirror() {
	std::string answer = receive("Cr51,1");
	return std::stoi(answer);
}

void Stand::setMirror(int position) {
	if (position > 0 && position < 3) {
		send("CR51," + std::to_string(position));
	}
	emit(elementPositionsChanged(5, position));
}

void Stand::setPreset(int preset) {
	setReflector(m_presets[preset][0]);
	setObjective(m_presets[preset][1]);
	setTubelens(m_presets[preset][2]);
	setBaseport(m_presets[preset][3]);
	setSideport(m_presets[preset][4]);
	setMirror(m_presets[preset][5]);
}
