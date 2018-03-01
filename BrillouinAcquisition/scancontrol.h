#ifndef SCANCONTROL_H
#define SCANCONTROL_H

#include <QSerialPort>

class com : public QSerialPort {
public:
	std::string receive(std::string request);
	void send(std::string message);
};

class Element {
private:
	std::string m_prefix;		// prefix of the element for serial communication
	com *m_comObject;
public:
	Element(com *comObject, std::string prefix) : m_comObject(comObject), m_prefix(prefix) {};
	~Element();
	std::string receive(std::string request);
	void send(std::string message);
	std::string dec2hex(int dec, int digits);
	int hex2dec(std::string);
};

class Stand : public Element {
public:
	Stand(com *comObject) : Element(comObject, "H") {};
	int getReflector();
	void setReflector(int position);

	int getObjective();
	void setObjective(int position);

	int getTubelens();
	void setTubelens(int position);

	int getBaseport();
	void setBaseport(int position);

	int getSideport();
	void setSideport(int position);

	int getMirror();
	void setMirror(int position);
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
	bool isConnected = FALSE;

public:
	ScanControl();
	~ScanControl();

	bool getConnectionStatus();

	void setPosition(std::vector<double> position);
	void setPositionRelative(std::vector<double> distance);
	std::vector<double> getPosition();

	Focus *focus;
	MCU *mcu;
	Stand *stand;

	com *m_comObject = new com();

public slots:
	bool connect();
	bool disconnect();

signals:
	void microscopeConnected(bool);
};

#endif // SCANCONTROL_H