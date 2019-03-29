#include "stdafx.h"
#include "MockMicroscope.h"

MockMicroscope::MockMicroscope() {
}

MockMicroscope::~MockMicroscope() {
	m_isOpen = false;
}

qint64 MockMicroscope::writeToDevice(const char *data) {
	std::string message = data;
	m_outputBuffer += message;
	return message.length();
}

std::string MockMicroscope::readOutputBuffer() {
	return m_outputBuffer;
}
