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
public:
	Element(com *comObject, std::string prefix) : m_comObject(comObject), m_prefix(prefix) {};
	~Element();
	std::string receive(std::string request);
	void send(std::string message);
	void setDevice(com *device);
};

class Stand : public Element {
	Q_OBJECT
public:
	Stand(com *comObject) : Element(comObject, "H") {};
	
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
	double m_umperinc = 0.025;		// [µm per increment] constant for converting µm to increments of focus z-position
	int m_rangeFocus = 16777215;	// number of focus increments

public:
	Focus(com *comObject) : Element(comObject, "F") {};
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
	double m_umperinc = 0.25;		// [µm per increment] constant for converting µm to increments of x- and y-position
	int m_rangeFocus = 16777215;	// number of focus increments

	double getPosition(std::string axis);
	void setPosition(std::string axis, double position);

	void setVelocity(std::string axis, int velocity);

public:
	MCU(com *comObject) : Element(comObject, "N") {};
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
	bool isConnected = false;

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
	bool connect();
	bool disconnect();
	void errorHandler(QSerialPort::SerialPortError error);

signals:
	void microscopeConnected(bool);
};

#endif // SCANCONTROL_H