#ifndef SCANCONTROL_H
#define SCANCONTROL_H

#include <QSerialPort>

class com : public QSerialPort {
public:
	std::string receive(std::string request);
	void send(std::string message);
	qint64 readLineDataCR(char *data, qint64 maxSize);
};

class Element : public QObject {
	Q_OBJECT
private:
	std::string m_prefix;		// prefix of the element for serial communication
	com *m_comObject;
public:
	Element(com *comObject, std::string prefix) : m_comObject(comObject), m_prefix(prefix) {};
	~Element();
	std::string parse(std::string answer);
	std::string receive(std::string request);
	void send(std::string message);
	std::string dec2hex(int dec, int digits);
	int hex2dec(std::string);
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
	bool isConnected = 0;

public:
	ScanControl();
	~ScanControl();

	bool getConnectionStatus();

	void setPosition(std::vector<double> position);
	void setPositionRelative(std::vector<double> distance);
	std::vector<double> getPosition();

	Focus *m_focus;
	MCU *m_mcu;
	Stand *m_stand;

	com *m_comObject = new com();

public slots:
	bool connect();
	bool disconnect();

signals:
	void microscopeConnected(bool);
};

#endif // SCANCONTROL_H