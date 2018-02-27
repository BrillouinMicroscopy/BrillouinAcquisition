#include "stdafx.h"
#include "scancontrol.h"
#include <iomanip>
#include <sstream>
#include <regex>

ScanControl::ScanControl() {
	focus = new Focus(m_comObject);
	mcu = new MCU(m_comObject);
	stand = new Stand(m_comObject);
}

ScanControl::~ScanControl() {
	delete focus;
	delete mcu;
	delete stand;
	delete m_comObject;
	disconnect();
}

bool ScanControl::connect() {
	if (!isConnected) {
		m_comObject->setBaudRate(QSerialPort::Baud9600);
		m_comObject->setPortName("COM1");
		m_comObject->setFlowControl(QSerialPort::HardwareControl);
		isConnected = m_comObject->open(QIODevice::ReadWrite);
	}
	return isConnected;
}

bool ScanControl::disconnect() {
	if (m_comObject && isConnected) {
		m_comObject->close();
		isConnected = FALSE;
	}
	return isConnected;
}

bool ScanControl::getConnectionStatus() {
	return isConnected;
}

void ScanControl::setPosition(std::vector<double> position) {
	mcu->setX(position[0]);
	mcu->setY(position[1]);
	focus->setZ(position[2]);
}

std::vector<double> ScanControl::getPosition() {
	double x = mcu->getX();
	double y = mcu->getY();
	double z = focus->getZ();
	return std::vector<double> {x, y, z};
}


/*
* Functions regarding the serial communication
*
*/

std::string ScanControl::com::receive(std::string request) {
	request = request + "\r";
	write(request.c_str());

	int toWrite = bytesToWrite();
	waitForBytesWritten();
	bool wasWritten = flush();

	waitForReadyRead();

	int toRead = bytesAvailable();

	QByteArray dataRead = readAll();

	std::string answer(dataRead.constData(), dataRead.length());

	return answer;
}

void ScanControl::com::send(std::string message) {
	message = message + "\r";
	write(message.c_str());
}


/*
* Functions of the parent class for all elements
*
*/

ScanControl::Element::~Element() {
}

std::string ScanControl::Element::receive(std::string request) {
	std::string answer = m_comObject->receive(m_prefix + "P" + request);
	std::string pattern = "([a-zA-Z\\d]*)\r";
	pattern = "P" + m_prefix + pattern;
	std::regex pieces_regex(pattern);
	std::smatch pieces_match;
	std::regex_match(answer, pieces_match, pieces_regex);
	return pieces_match[1]; // needs error handling
}

void ScanControl::Element::send(std::string message) {
	m_comObject->send(m_prefix + "P" + message);
}

std::string ScanControl::Element::dec2hex(int dec, int digits = 6) {
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

int ScanControl::Element::hex2dec(std::string s) {
	return std::stoul(s, nullptr, 16);
}


/*
* Functions regarding the objective focus
*
*/

double ScanControl::Focus::getZ() {
	std::string s = "0x" + receive("Zp");
	return m_umperinc * hex2dec(s);
}

void ScanControl::Focus::setZ(double position) {
}


/*
 * Functions regarding the stage of the microscope
 *
 */

double ScanControl::MCU::getPosition(std::string axis) {
	std::string position = receive(axis + "p");
	int pos = hex2dec(position);
	return pos * m_umperinc;
}

void ScanControl::MCU::setPosition(std::string axis, double position) {
	position = round(position / m_umperinc);
	int inc = static_cast<int>(position);
	inc %= m_rangeFocus;
	std::string pos = dec2hex(inc, 6);
	send(axis + "T" + pos);
}

void ScanControl::MCU::setVelocity(std::string axis, int velocity) {
	std::string vel = dec2hex(velocity, 6);
	send(axis + "V" + vel);
}

double ScanControl::MCU::getX() {
	return getPosition("X");
}

void ScanControl::MCU::setX(double position) {
	setPosition("X", position);
}

double ScanControl::MCU::getY() {
	return getPosition("Y");
}

void ScanControl::MCU::setY(double position) {
	setPosition("Y", position);
}

void ScanControl::MCU::setVelocityX(int velocity) {
	setVelocity("X", velocity);
}

void ScanControl::MCU::setVelocityY(int velocity) {
	setVelocity("Y", velocity);
}

void ScanControl::MCU::stopX() {
	send("XS");
}

void ScanControl::MCU::stopY() {
	send("YS");
}


/*
 * Functions regarding the stand of the microscope
 *
 */

int ScanControl::Stand::getReflector() {
	std::string answer = receive("Cr1,1");
	return std::stoi(answer);
}

void ScanControl::Stand::setReflector(int position) {
	if (position > 0 && position < 6) {
		send("CR1," + std::to_string(position));
	}
}

int ScanControl::Stand::getObjective() {
	std::string answer = receive("Cr2,1");
	return std::stoi(answer);
}

void ScanControl::Stand::setObjective(int position) {
	if (position > 0 && position < 7) {
		send("CR2," + std::to_string(position));
	}
}

int ScanControl::Stand::getTubelens() {
	std::string answer = receive("Cr36,1");
	return std::stoi(answer);
}

void ScanControl::Stand::setTubelens(int position) {
	if (position > 0 && position < 4) {
		send("CR36," + std::to_string(position));
	}
}

int ScanControl::Stand::getBaseport() {
	std::string answer = receive("Cr38,1");
	return std::stoi(answer);
}

void ScanControl::Stand::setBaseport(int position) {
	if (position > 0 && position < 4) {
		send("CR38," + std::to_string(position));
	}
}

int ScanControl::Stand::getSideport() {
	std::string answer = receive("Cr39,1");
	return std::stoi(answer);
}

void ScanControl::Stand::setSideport(int position) {
	if (position > 0 && position < 4) {
		send("CR39," + std::to_string(position));
	}
}

int ScanControl::Stand::getMirror() {
	std::string answer = receive("Cr51,1");
	return std::stoi(answer);
}

void ScanControl::Stand::setMirror(int position) {
	if (position > 0 && position < 3) {
		send("CR51," + std::to_string(position));
	}
}
