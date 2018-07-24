#ifndef ZEISSECU_H
#define ZEISSECU_H

#include <QSerialPort>
#include "scancontrol.h"

class helper {
public:
	static std::string dec2hex(int dec, int digits);
	static int hex2dec(std::string);
	static std::string parse(std::string answer, std::string prefix);
};

class com : public QSerialPort {
protected:
	std::string m_terminator = "\r";
public:
	std::string receive(std::string request);
	void send(std::string message);

	virtual qint64 writeToDevice(const char *data);
};

class Element : public QObject {
	Q_OBJECT
private:
	std::string m_prefix;		// prefix of the element for serial communication
	com *m_comObject;
	std::vector<std::string> m_versions;

public:
	Element(com *comObject, std::string prefix, std::vector<std::string> versions) : m_comObject(comObject), m_prefix(prefix), m_versions(versions) {};
	~Element();
	std::string receive(std::string request);
	void send(std::string message);
	void setDevice(com *device);
	inline int positive_modulo(int i, int n) {
		return (i % n + n) % n;
	}
	std::string requestVersion();
	bool checkCompatibility();
};

class Stand : public Element {
	Q_OBJECT
public:
	Stand(com *comObject) : Element(comObject, "H", { "AV_V3_17" }) {};

	void setReflector(int position);
	int getReflector();
	void setObjective(int position);
	int getObjective();
	void setTubelens(int position);
	int getTubelens();
	void setBaseport(int position);
	int getBaseport();
	void setSideport(int position);
	int getSideport();
	void setMirror(int position);
	int getMirror();
};

class Focus : public Element {
private:
	double m_umperinc = 0.025;		// [�m per increment] constant for converting �m to increments of focus z-position
	int m_rangeFocus = 16777215;	// number of focus increments

public:
	Focus(com *comObject) : Element(comObject, "F", { "ZM_V2_04" }) {};
	double getZ();
	void setZ(double position);

	void setVelocityZ(double velocity);

	void scanUp();
	void scanDown();
	void scanStop();
	int getScanStatus();
	int getStatusKey();
	void move2Load();
	void move2Work();
};

class MCU : public Element {
private:
	double m_umperinc = 0.25;		// [�m per increment] constant for converting �m to increments of x- and y-position
	int m_rangeFocus = 16777215;	// number of focus increments

	double getPosition(std::string axis);
	void setPosition(std::string axis, double position);

	void setVelocity(std::string axis, int velocity);

public:
	MCU(com *comObject) : Element(comObject, "N", { "MC V2.08" }) {};
	double getX();
	void setX(double position);

	double getY();
	void setY(double position);

	void setVelocityX(int velocity);
	void setVelocityY(int velocity);

	void stopX();
	void stopY();
};

class ZeissECU: public ScanControl {
	Q_OBJECT

private:
	com * m_comObject = nullptr;

	Focus *m_focus = nullptr;
	MCU *m_mcu = nullptr;
	Stand *m_stand = nullptr;

public:
	ZeissECU() noexcept;
	~ZeissECU();

	void setPosition(POINT3 position);
	void setPositionRelative(POINT3 distance);
	POINT3 getPosition();
	void setDevice(com *device);

public slots:
	void init();
	bool connectDevice();
	bool disconnectDevice();
	void errorHandler(QSerialPort::SerialPortError error);
	void setElement(ScanControl::DEVICE_ELEMENT element, int position);
	void setElements(ScanControl::SCAN_PRESET preset);
	void getElements();
};

#endif // ZEISSECU_H