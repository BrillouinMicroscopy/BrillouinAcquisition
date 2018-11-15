#ifndef NIDAQ_H
#define NIDAQ_H

#include <QSerialPort>
#include <vector>
#include "NIDAQmx.h"
#include "scancontrol.h"

namespace Thorlabs_TIM {
	#include <Thorlabs.MotionControl.TCube.InertialMotor.h>
}
namespace Thorlabs_FF {
	#include <Thorlabs.MotionControl.FilterFlipper.h>
}
namespace Thorlabs_KSC {
	#include <Thorlabs.MotionControl.KCube.Solenoid.h>
}
namespace Thorlabs_KDC {
	#include <Thorlabs.MotionControl.KCube.DCServo.h>
}
#include "filtermount.h"

#include "H5Cpp.h"
#include "filesystem"

struct ACQ_VOLTAGES {
	int numberSamples{ 0 };
	std::vector<float64> mirror;
	std::vector<uInt8> trigger;
};

class NIDAQ: public ScanControl {
	Q_OBJECT

private:
	TaskHandle AOtaskHandle = 0;
	TaskHandle DOtaskHandle = 0;
	TaskHandle DOtaskHandle_LED = 0;

	struct TTL {
		const uInt8 low = 0;
		const uInt8 high = 1;
	} m_TTL;
	
	struct Calibration {
		std::string date{ "" };
		POINT2 translation{ -3.8008e-6, 1.1829e-6 };	// [m]	translation
		double rho{ -0.2528 };		// [rad]	rotation
		double fliplr{ 1 };
		double flipud{ 1 };
		COEFFICIENTS5 coef{
			-6.9185e-4, // [1/m³]	coefficient of fourth order
			6.7076e-4,	// [1/m²]	coefficient of third order
			-1.1797e-4,	// [1/m]	coefficient of second order
			4.1544e-4,	// [1]		coefficient of first order
			0			// [m]		offset term
		};
		BOUNDS bounds = {
			-53,	// [µm] minimal x-value
			 53,	// [µm] maximal x-value
			-43,	// [µm] minimal y-value
			 43,	// [µm] maximal y-value
			 -1000,	// [µm] minimal z-value
			  1000	// [µm] maximal z-value
		};
		bool valid = false;
	} m_calibration;

	VOLTAGE2 m_voltages{ 0, 0 };	// current voltage
	POINT3 m_position{ 0, 0, 0 };	// current position
	bool m_LEDon{ false };			// current state of the LED illumination source
	
	
	// TODO: make the following parameters changeable:
	char const *m_serialNo_TIM = "65864438";	// serial number of the TCube Inertial motor controller device (can be found in Kinesis)
	Thorlabs_TIM::TIM_Channels m_channelPosZ{ Thorlabs_TIM::Channel1 };
	int m_PiezoIncPerMum{ 50 };

	char const *m_serialNo_FF1 = "37000784";
	char const *m_serialNo_KSC = "68000952";
	
	char const *m_serialNo_KDC = "27503225";
	// see https://www.thorlabs.com/drawings/279d37ef141e2423-056D0D56-F367-26BE-7B83AD99FE5D61F2/Z825B-Manual.pdf, page 9
	double m_stepsPerRev{ 512 };	// [1]  steps per revelation
	double m_gearBoxRatio{ 67 };	// [1]  ratio of the gear box
	double m_pitch{ 1 };			// [mm] pitch of the lead screw

	// moveable filter mounts
	FilterMount *m_exFilter = nullptr;
	FilterMount *m_emFilter = nullptr;

	enum class DEVICE_ELEMENT {
		BEAMBLOCK,
		CALFLIPMIRROR,
		MOVEMIRROR,
		EXFILTER,
		EMFILTER,
		LEDLAMP,
		COUNT
	};

public:
	NIDAQ() noexcept;
	~NIDAQ();

	VOLTAGE2 positionToVoltage(POINT2 position);
	POINT2 voltageToPosition(VOLTAGE2 position);

	void applyScanPosition();

	void setPosition(POINT3 position);
	POINT3 getPosition();

	void setVoltage(VOLTAGE2 voltage);

	// NIDAQ specific function to move position to center of field of view
	void centerPosition();

public slots:
	void init();
	void connectDevice();
	void disconnectDevice();
	void setElement(DeviceElement element, int position);
	void getElement(DeviceElement element);
	void setPreset(SCAN_PRESET preset);
	void getElements();
	void setCalFlipMirror(int position);
	void setBeamBlock(int position);
	void setMirror(int position);
	void setEmFilter(int position);
	void setExFilter(int position);
	void setFilter(FilterMount * device, int position);
	int getBeamBlock();
	int getExFilter();
	int getEmFilter();
	int getFilter(FilterMount * device);
	void setLEDLamp(bool position);
	int getLEDLamp();
	int getMirror();
	// sets the position relative to the home position m_homePosition
	void setPositionRelativeX(double position);
	void setPositionRelativeY(double position);
	void setPositionRelativeZ(double position);
	void loadVoltagePositionCalibration(std::string filepath) override;
	double getCalibrationValue(H5::H5File file, std::string datasetName);

	void triggerCamera();
	void setAcquisitionVoltages(ACQ_VOLTAGES voltages);
};

#endif // NIDAQMX_H