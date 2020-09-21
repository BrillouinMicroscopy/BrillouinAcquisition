#ifndef COM_H
#define COM_H

#include <sstream>
#include <iomanip>
#include <regex>

#include <QSerialPort>
#include <QtCore>
#include <gsl/gsl>

class com : public QSerialPort {
public:
	com() {};
	com(std::string terminator) : m_terminator(terminator) {};

	virtual qint64 writeToDevice(const char* data);

	std::string receive(std::string request);
	void send(std::string message);

protected:
	std::string m_terminator{ "\r" };
	bool waitForReady(int timeout);
};

class helper {
public:
	static std::string dec2hex(int dec, int digits);
	static int hex2dec(std::string);
	static std::string parse(std::string answer, std::string prefix);
};

#endif //COM_H