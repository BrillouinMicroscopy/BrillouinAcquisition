#ifndef SCANCONTROL_H
#define SCANCONTROL_H

#include <QSerialPort>

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
	
	int getReflector();
	int getObjective();
	int getTubelens();
	int getBaseport();
	int getSideport();
	int getMirror();

	std::vector<std::vector<int>> m_presets = {
		{ 1, 1, 3, 1, 2, 1 },	// Brillouin
		{ 1, 1, 3, 1, 2, 2 },	// Brightfield
		{ 1, 1, 3, 2, 3, 2 },	// Eyepiece
		{ 1, 1, 3, 1, 3, 2 }	// Calibration
	};

public slots:
	void getElementPositions();
	void setReflector(int position);
	void setObjective(int position);
	void setTubelens(int position);
	void setBaseport(int position);
	void setSideport(int position);
	void setMirror(int position);
	void setPreset(int preset);

signals:
	void elementPositionsChanged(std::vector<int>);
	void elementPositionsChanged(int, int);
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

class ScanControl: public QObject {
	Q_OBJECT

private:
	bool m_isConnected = false;
	bool m_isCompatible = false;

public:
	ScanControl();
	~ScanControl();

	bool getConnectionStatus();

	void setPosition(std::vector<double> position);
	void setPositionRelative(std::vector<double> distance);
	std::vector<double> getPosition();
	void setDevice(com *device);

	com *m_comObject = new com();

	Focus *m_focus = new Focus(m_comObject);
	MCU *m_mcu = new MCU(m_comObject);
	Stand *m_stand = new Stand(m_comObject);

public slots:
	bool connectDevice();
	bool disconnectDevice();
	void errorHandler(QSerialPort::SerialPortError error);

signals:
	void microscopeConnected(bool);
};

#endif // SCANCONTROL_H