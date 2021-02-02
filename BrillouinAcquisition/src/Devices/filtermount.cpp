#include "stdafx.h"
#include "filtermount.h"

void FilterMount::init() {
	m_comObject = new com("\r\n");
}

FilterMount::~FilterMount() {
	disconnectDevice();
	if (m_comObject) {
		m_comObject->deleteLater();
	}
}

void FilterMount::connectDevice() {
	if (!m_isConnected) {
		try {
			m_comObject->setPortName(m_comPort);
			if (!m_comObject->setBaudRate(QSerialPort::Baud9600)) {
				throw QString("Could not set BaudRate.");
			}
			if (!m_comObject->setFlowControl(QSerialPort::NoFlowControl)) {
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
		} catch (QString e) {
			// todo
		}
	}
	emit(connectedDevice(m_isConnected));
}

void FilterMount::disconnectDevice() {
	if (m_comObject && m_isConnected) {
		m_comObject->close();
		m_isConnected = false;
	}
	emit(connectedDevice(m_isConnected));
}

void FilterMount::home() {
	auto tmp = m_comObject->receive("0ho0");
}

double FilterMount::getPosition() {
	std::string posString = m_comObject->receive("0gp");
	std::string parsed = parsePosition(posString);
	return helper::hex2dec(parsed);
}

void FilterMount::setPosition(double position) {
	std::string pos = helper::dec2hex(position, 8);
	std::string posString = m_comObject->receive("0ma" + pos);
}

void FilterMount::moveForward() {
	auto tmp = m_comObject->receive("0fw");
}

void FilterMount::moveBackward() {
	auto tmp = m_comObject->receive("0bw");
}

std::string FilterMount::parsePosition(std::string position) {
	std::string pattern = "([a-zA-Z\\d\\s_.]*)\r\n";
	pattern = "0PO" + pattern;
	std::regex pieces_regex(pattern);
	std::smatch pieces_match;
	std::regex_match(position, pieces_match, pieces_regex);
	if (pieces_match.size() == 0) {
		return "";
	} else {
		return pieces_match[1];
	}
}