#ifndef NIDAQ_H
#define NIDAQ_H

#include <QSerialPort>
#include <vector>
#include "NIDAQmx.h"
#include "ODTControl.h"

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

class NIDAQ: public ODTControl {
	Q_OBJECT

public:
	NIDAQ() noexcept;
	~NIDAQ();

	void setPosition(POINT2 position) override;
	void setPosition(POINT3 position) override;
	POINT3 getPosition() override;

	void setVoltage(VOLTAGE2 voltage) override;

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

	void setSpatialCalibration(SpatialCalibration spatialCalibration) override;
	
	void setAcquisitionVoltages(ACQ_VOLTAGES voltages) override;

	void setHome();

private:
	POINT2 pixToMicroMeter(POINT2) override;

	VOLTAGE2 positionToVoltage(POINT2 position);
	POINT2 voltageToPosition(VOLTAGE2 position);

	void applyScanPosition();
	void centerPosition();

	void setFilter(FilterMount* device, int position);
	int getFilter(FilterMount* device);

	void setCalFlipMirror(int position);
	int getCalFlipMirror();
	void setBeamBlock(int position);
	int getBeamBlock();
	void setMirror(int position);
	int getMirror();
	void setEmFilter(int position);
	int getEmFilter();
	void setExFilter(int position);
	int getExFilter();
	void setLowerObjective(double position);
	double getLowerObjective();
	void setLEDLamp(bool position);
	int getLEDLamp();

	void triggerCamera();

	VOLTAGE2 m_voltages{ 0, 0 };	// current voltage
	POINT3 m_position{ 0, 0, 0 };	// current position
	bool m_LEDon{ false };			// current state of the LED illumination source
	double m_positionLowerObjective{ 0 };	// position of the lower objective
	
	// TODO: make the following parameters changeable:
	char const* m_serialNo_TIM{ "65864438" };	// serial number of the TCube Inertial motor controller device (can be found in Kinesis)
	Thorlabs_TIM::TIM_Channels m_channelPosZ{ Thorlabs_TIM::Channel1 };
	Thorlabs_TIM::TIM_Channels m_channelLowerObjective{ Thorlabs_TIM::Channel2 };
	int m_PiezoIncPerMum{ 50 };

	char const* m_serialNo_FF1{ "37000784" };
	char const* m_serialNo_KSC{ "68000952" };
	
	char const *m_serialNo_KDC{ "27503225" };
	// see https://www.thorlabs.com/drawings/279d37ef141e2423-056D0D56-F367-26BE-7B83AD99FE5D61F2/Z825B-Manual.pdf, page 9
	double m_stepsPerRev{ 512 };	// [1]  steps per revelation
	double m_gearBoxRatio{ 67 };	// [1]  ratio of the gear box
	double m_pitch{ 1 };			// [mm] pitch of the lead screw

	// moveable filter mounts
	FilterMount* m_exFilter{ nullptr };
	FilterMount* m_emFilter{ nullptr };

	enum class DEVICE_ELEMENT {
		BEAMBLOCK,
		CALFLIPMIRROR,
		MOVEMIRROR,
		EXFILTER,
		EMFILTER,
		LEDLAMP,
		LOWEROBJECTIVE,
		COUNT
	};
};

#endif // NIDAQMX_H