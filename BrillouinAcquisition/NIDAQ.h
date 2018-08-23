#ifndef NIDAQ_H
#define NIDAQ_H

#include <QSerialPort>
#include <vector>
#include "NIDAQmx.h"
#include "scancontrol.h"
#include "simplemath.h"
#include <gsl/gsl>
#include <Thorlabs.MotionControl.TCube.InertialMotor.h>

#include "H5Cpp.h"
#include "filesystem"

struct VOLTAGE2 {
	double Ux{ 0 };
	double Uy{ 0 };
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
		std::string date{ "" };
		POINT2 translation{ -3.8008e-6, 1.1829e-6 };	// [m]	translation
		double rho{ -0.2528 };		// [rad]	rotation
		COEFFICIANTS5 coef{
			-6.9185e-4, // [1/m�]	coefficient of fourth order
			6.7076e-4,	// [1/m�]	coefficient of third order
			-1.1797e-4,	// [1/m]	coefficient of second order
			4.1544e-4,	// [1]		coefficient of first order
			0			// [m]		offset term
		};
		BOUNDS bounds = {
			-53,	// [�m] minimal x-value
			 53,	// [�m] maximal x-value
			-43,	// [�m] minimal y-value
			 43,	// [�m] maximal y-value
			 -1000,	// [�m] minimal z-value
			  1000	// [�m] maximal z-value
		};
		bool valid = false;
	} m_calibration;

	VOLTAGE2 m_voltages{ 0, 0 };	// current voltage
	POINT3 m_position{ 0, 0, 0 };	// current position
	
	// TODO: make the following parameters changeable:
	char const *m_serialNo = "65864438";	// serial number of the TCube Inertial motor controller device (can be found in Kinesis)
	TIM_Channels m_channelPosZ{ Channel1 };

public:
	NIDAQ() noexcept;
	~NIDAQ();

	VOLTAGE2 positionToVoltage(POINT2 position);
	POINT2 voltageToPosition(VOLTAGE2 position);

	void setPosition(POINT3 position);
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
	void loadVoltagePositionCalibration(std::string filepath) override;
	double getCalibrationValue(H5::H5File file, std::string datasetName);
};

#endif // NIDAQMX_H