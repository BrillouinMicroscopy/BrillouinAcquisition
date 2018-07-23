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

	void setPosition(std::vector<double> position);
	void setPositionRelative(std::vector<double> distance);
	std::vector<double> getPosition();

public slots:
	void init();
	bool connectDevice();
	bool disconnectDevice();
	void setElement(ScanControl::DEVICE_ELEMENT element, int position);
	void setElements(ScanControl::SCAN_PRESET preset);
	void getElements();
};

#endif // NIDAQMX_H