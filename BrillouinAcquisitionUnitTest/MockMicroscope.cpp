#include "stdafx.h"
#include "MockMicroscope.h"

MockMicroscope::MockMicroscope() {
}

MockMicroscope::~MockMicroscope() {
	m_isOpen = 0;
}

bool MockMicroscope::open(OpenMode mode) {
	m_mode = mode;
	m_isOpen = 1;
	return m_isOpen;
}

void MockMicroscope::close() {
	m_isOpen = 0;
}

qint64 MockMicroscope::readCharacter(char *data, qint64 maxlen) {
	if (m_outputBuffer.size() > 0) {
		char tmp = m_outputBuffer[0];
		strcpy(data, &tmp);
		m_outputBuffer.remove(0, 1);
		return 1;
	} else {
		return 0;
	}
}

qint64 MockMicroscope::writeData(const char *data, qint64 maxSize) {
	std::string request = data;
	std::string answer;
	if (request == "FPZp\r")
		answer = "PF000000\r";
	if (request == "NPXp\r")
		answer = "PN000000\r";
	if (request == "NPYp\r")
		answer = "PN000000\r";
	m_outputBuffer = answer.c_str();
	return 1;
}

bool MockMicroscope::waitForBytesWritten(int msecs) {
	return true;
}

bool MockMicroscope::waitForReadyRead(int msecs) {
	return true;
}
