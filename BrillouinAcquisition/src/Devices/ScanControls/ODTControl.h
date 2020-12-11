#ifndef ODTCONTROL_H
#define ODTCONTROL_H

#include <QSerialPort>
#include <vector>
#include "NIDAQmx.h"
#include "ScanControl.h"
#include "../../Acquisition/AcquisitionModes/VoltageCalibrationHelper.h"

#include "H5Cpp.h"
#include "filesystem"

struct ACQ_VOLTAGES {
	int numberSamples{ 0 };
	std::vector<float64> mirror;
	std::vector<uInt8> trigger;
};

struct TTL {
	const uInt8 low{ 0 };
	const uInt8 high{ 1 };
};

struct VOLTAGE2 {
	double Ux{ 0 };
	double Uy{ 0 };
};

class ODTControl: public ScanControl {
	Q_OBJECT

public:
	ODTControl() noexcept {};
	~ODTControl() {};

	void setVoltage(VOLTAGE2 voltages);

public slots:
	void connectDevice();
	void disconnectDevice();

	void setLEDLamp(bool enabled);
	int getLEDLamp();

	void setAcquisitionVoltages(ACQ_VOLTAGES voltages);
	virtual void setVoltageCalibration(VoltageCalibrationData voltageCalibration) {};

protected:
	TaskHandle AOtaskHandle{ 0 };
	TaskHandle DOtaskHandle{ 0 };
	TaskHandle DOtaskHandle_LED{ 0 };

	TTL m_TTL;

	VOLTAGE2 m_voltages{ 0, 0 };	// current voltage

	VoltageCalibrationData m_voltageCalibration;

private:
	bool m_LEDon{ false };			// current state of the LED illumination source
};

#endif // ODTCONTROL_H