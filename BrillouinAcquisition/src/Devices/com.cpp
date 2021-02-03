#include "stdafx.h"
#include "com.h"

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
		if (waitForReady(1000)) {
			QByteArray responseData = readAll();
			while (waitForReady(50))
				responseData += readAll();

			response = responseData;
		}
	}

	return response;
}

bool com::waitForReady(int timeout) {
	waitForReadyRead(5);
	QElapsedTimer elapsed;
	elapsed.start();
	while (bytesAvailable() == 0 && elapsed.elapsed() < timeout) {
		waitForReadyRead(5);
	}
	if (elapsed.elapsed() < timeout) {
		return true;
	}
	return false;
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

std::string helper::parse(std::string answer, const std::string& prefix) {
	std::string pattern = "([a-zA-Z\\d\\s_.]*)\r";
	pattern = "P" + prefix + pattern;
	std::regex pieces_regex(pattern);
	std::smatch pieces_match;
	std::regex_match(answer, pieces_match, pieces_regex);
	if (pieces_match.size() == 0) {
		return "";
	} else {
		return pieces_match[1];
	}
}