#ifndef NIDAQ_H
#define NIDAQ_H

#include <QSerialPort>
#include <vector>
#include "NIDAQmx.h"
#include "scancontrol.h"
#include "simplemath.h"
#include <gsl/gsl>

struct VOLTAGE2 {
	double Ux{ 0 };
	double Uy{ 0 };
};

struct Bounds {
	double xMin{ -53e-6 };	// [m] minimal x-value
	double xMax{  53e-6 };	// [m] maximal x-value
	double yMin{ -43e-6 };	// [m] minimal y-value
	double yMax{  43e-6 };	// [m] maximal y-value
};

struct POINT2 {
	double x{ 0 };
	double y{ 0 };
	POINT2 operator+(const POINT2 &pos) {
		return POINT2{ x + pos.x, y + pos.y };
	}
	POINT2 operator-(const POINT2 &pos) {
		return POINT2{ x - pos.x, y - pos.y };
	}
};

class NIDAQ: public ScanControl {
	Q_OBJECT

private:
	TaskHandle taskHandle = 0;
	
	struct Calibration {
		POINT2 translation{ -3.8008e-6, 1.1829e-6 };	// [µm]	translation
		double rho{ -0.2528 };		// [rad]	rotation
		COEFFICIANTS5 coef{
			-6.9185e-4, // [1/m³]	coefficient of fourth order
			6.7076e-4,	// [1/m²]	coefficient of third order
			-1.1797e-4,	// [1/m]	coefficient of second order
			4.1544e-4,	// [1]		coefficient of first order
			0			// [m]		offset term
		};
		Bounds bounds;
		bool valid = false;
	} m_calibration;

	VOLTAGE2 m_voltages{ 0, 0 };	// current voltage
	POINT3 m_position{ 0, 0, 0 };	// current position

public:
	NIDAQ() noexcept;
	~NIDAQ();

	VOLTAGE2 positionToVoltage(POINT2 position);
	POINT2 voltageToPosition(VOLTAGE2 position);

	void setPosition(POINT3 position);
	// moves the position relative to current position
	void movePosition(POINT3 distance);
	POINT3 getPosition();

	// NIDAQ specific function to move position to center of field of view
	void centerPosition();

public slots:
	void init();
	bool connectDevice();
	bool disconnectDevice();
	void setElement(ScanControl::DEVICE_ELEMENT element, int position);
	void setElements(ScanControl::SCAN_PRESET preset);
	void getElements();
	// sets the position relative to the home position m_homePosition
	void setPositionRelativeX(double position);
	void setPositionRelativeY(double position);
	void setPositionRelativeZ(double position);
	void loadCalibration(std::string filepath);
};

#endif // NIDAQMX_H