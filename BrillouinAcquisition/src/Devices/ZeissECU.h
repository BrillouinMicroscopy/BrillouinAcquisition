#ifndef ZEISSECU_H
#define ZEISSECU_H

#include "scancontrol.h"
#include "com.h"

namespace Thorlabs_FF {
	#include <Thorlabs.MotionControl.FilterFlipper.h>
}

class Element : public QObject {
	Q_OBJECT

public:
	Element(com *comObject, std::string prefix, std::vector<std::string> versions) : m_comObject(comObject), m_prefix(prefix), m_versions(versions) {};
	~Element() {};
	void setDevice(com* device);
	bool checkCompatibility();

protected:
	std::string receive(std::string request);
	void send(std::string message);
	void clear();
	std::string requestVersion();

	inline int positive_modulo(int i, int n) {
		return (i % n + n) % n;
	}

private:
	std::string m_prefix;		// prefix of the element for serial communication
	com* m_comObject;
	std::vector<std::string> m_versions;
};

class Stand : public Element {
	Q_OBJECT
public:
	Stand(com *comObject) : Element(comObject, "H", { "AV_V3_17" }) {};

	void setReflector(int position, bool check = false);
	int getReflector();
	void setObjective(int position, bool check = false);
	int getObjective();
	void setTubelens(int position, bool check = false);
	int getTubelens();
	void setBaseport(int position, bool check = false);
	int getBaseport();
	void setSideport(int position, bool check = false);
	int getSideport();
	void setMirror(int position, bool check = false);
	int getMirror();

private:
	void setElementPosition(std::string device, int position);
	int getElementPosition(std::string device);
	void blockUntilPositionReached(bool block, std::string elementNr);
};

class Focus : public Element {

public:
	Focus(com *comObject) : Element(comObject, "F", { "ZM_V2_04" }) {};

	void setZ(double position);
	double getZ();

	void setVelocityZ(double velocity);

	void scanUp();
	void scanDown();
	void scanStop();
	int getScanStatus();
	int getStatusKey();
	void move2Load();
	void move2Work();

private:
	double m_umperinc{ 0.025 };		// [µm per increment] constant for converting µm to increments of focus z-position
	int m_rangeFocus{ 16777215 };	// number of focus increments
};

class MCU : public Element {

public:
	MCU(com *comObject) : Element(comObject, "N", { "MC V2.08" }) {};

	void setX(double position);
	double getX();

	void setY(double position);
	double getY();

	void setVelocityX(int velocity);
	void setVelocityY(int velocity);

	void stopX();
	void stopY();

private:
	void setPosition(std::string axis, double position);
	double getPosition(std::string axis);

	void setVelocity(std::string axis, int velocity);

	double m_umperinc{ 0.25 };		// [µm per increment] constant for converting µm to increments of x- and y-position
	int m_rangeFocus{ 16777215 };	// number of focus increments
};

class ZeissECU: public ScanControl {
	Q_OBJECT

public:
	ZeissECU() noexcept;
	~ZeissECU();

	void setPosition(POINT3 position) override;
	void setPosition(POINT2 position) override;
	POINT3 getPosition() override;

	void setDevice(com *device);

public slots:
	void init() override;
	void connectDevice() override;
	void disconnectDevice() override;
	void setElement(DeviceElement element, double position) override;
	int getElement(DeviceElement element) override;
	void getElements() override;
	void setPreset(ScanPreset preset) override;

	// sets the position relative to the home position m_homePosition
	void setPositionRelativeX(double position) override;
	void setPositionRelativeY(double position) override;
	void setPositionRelativeZ(double position) override;
	void setPositionInPix(POINT2) override;

private:
	POINT2 pixToMicroMeter(POINT2) override;

	void setBeamBlock(int position);
	int getBeamBlock();

	void errorHandler(QSerialPort::SerialPortError error);

	com* m_comObject{ nullptr };

	Focus* m_focus{ nullptr };
	MCU* m_mcu{ nullptr };
	Stand* m_stand{ nullptr };

	char const* m_serialNo_FF2{ "37000251" };

	enum class DEVICE_ELEMENT {
		BEAMBLOCK,
		OBJECTIVE,
		REFLECTOR,
		TUBELENS,
		BASEPORT,
		SIDEPORT,
		MIRROR,
		COUNT
	};
};

#endif // ZEISSECU_H