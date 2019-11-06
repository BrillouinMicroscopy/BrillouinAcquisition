#ifndef ODTCONTROL_H
#define ODTCONTROL_H

#include <QSerialPort>
#include <vector>
#include "NIDAQmx.h"
#include "scancontrol.h"

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
	ODTControl() noexcept;
	~ODTControl();

	virtual void setVoltage(VOLTAGE2 voltage) = 0;

public slots:
	virtual void setLEDLamp(bool enabled) = 0;
	virtual void setAcquisitionVoltages(ACQ_VOLTAGES voltages) = 0;

protected:
	TaskHandle AOtaskHandle{ 0 };
	TaskHandle DOtaskHandle{ 0 };
	TaskHandle DOtaskHandle_LED{ 0 };

	TTL m_TTL;

	VOLTAGE2 m_voltages{ 0, 0 };	// current voltage
	POINT3 m_position{ 0, 0, 0 };	// current position
};

#endif // ODTCONTROL_H