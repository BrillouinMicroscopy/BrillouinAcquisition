#ifndef SCANCONTROL_H
#define SCANCONTROL_H

#include <QSerialPort>

class ScanControl {
private:
	class com {
	private:
		QSerialPort serialObject;
	public:
		std::string receive(std::string request);
		void send();
	};

	com *m_comObject;

	class Element {
	private:
		std::string m_prefix;		// prefix of the element for serial communication
		com *m_comObject;
	public:
		Element(com *comObject, std::string prefix) : m_comObject(comObject), m_prefix(prefix) {};
		~Element();
		std::string receive(std::string request);
		void send();
		std::string dec2hex(int dec);
		int hex2dec(std::string);
	};

	class Focus : public Element {
	private:
		std::string m_prefix = "F";
		double m_umperinc = 0.025;		// [µm per increment] constant for converting µm to increments of focus z-position
		double m_rangeFocus = 16777215;	// number of focus increments

	public:
		Focus(com *comObject);
		double getZ();
		void setZ(double position);

		void setVelocity(double velocity);

		void scanUp();
		void scanDown();
		void scanStop();
		void getScanStatus();
		double getStatusKey();
		void move2Load();
		void move2Work();
	};

	class MCU : public Element {
	private:
		std::string m_prefix = "N";

	public:
		MCU(com *comObject);
		double getX();
		void setX(double position);

		double getY();
		void setY(double position);

		void setVelocityX(double velocity);
		void setVelocityY(double velocity);

		void stopX();
		void stopY();
	};

	class Stand : public Element {
	private:
		std::string m_prefix = "H";

	public:
		Stand(com *comObject);
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

	Focus *focus;
	MCU *mcu;
	Stand *stand;

public:
	ScanControl();
	~ScanControl();

	void connect();
	void disconnect();

	void setPosition(std::vector<double> position);
	std::vector<double> getPosition();

	void moveRelative(std::vector<double> distance);
	void moveAbsolute(std::vector<double> position);
};

#endif // SCANCONTROL_H