#ifndef NIDAQ_H
#define NIDAQ_H

#include <QSerialPort>
#include <vector>
#include "NIDAQmx.h"
#include "scancontrol.h"

class NIDAQ: public ScanControl {
	Q_OBJECT

private:
	TaskHandle taskHandle = 0;
	// very basic functions to convert position to voltage and vice versa
	// this actually needs calibration and a look-up-table, must do for now
	double positionToVoltage(double position);
	double voltageToPosition(double position);
	double m_mumPerVolt = 100;

	struct Voltages {
		double chA{ 0 };
		double chB{ 0 };
	} voltages;

public:
	NIDAQ() noexcept;
	~NIDAQ();

	void setPosition(POINT3 position);
	// moves the position relative to current position
	void movePosition(POINT3 distance);
	POINT3 getPosition();

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
};

#endif // NIDAQMX_H