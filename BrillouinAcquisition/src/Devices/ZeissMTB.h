#ifndef ZEISSMTB_H
#define ZEISSMTB_H

#include <atlutil.h>
#include "scancontrol.h"
#include "ZeissECU.h"
#import "MTBApi.tlb" named_guids
using namespace MTBApi;

class ZeissMTB: public ScanControl {
	Q_OBJECT

private:
	com * m_comObject = nullptr;

	Focus *m_focus = nullptr;
	MCU *m_mcu = nullptr;
	Stand *m_stand = nullptr;

	/*
	 * Zeiss MTB handles
	 */
	// MTB interface pointer to the connection class
	IMTBConnectionPtr m_MTBConnection = nullptr;
	// MTB interface ptr to the root of the tree of devices of the microscope
	IMTBRootPtr m_Root = nullptr;
	// my ID received from MTB
	CComBSTR m_ID = _T("");
	// MTB interface pointer to a device
	IMTBDevicePtr m_Stand = nullptr;
	// MTB interface pointer to a lamp
	IMTBContinualPtr m_Lamp = nullptr;


	bool m_isMTBConnected{ false };

	char const *m_serialNo_FF2 = "37000251";

	enum class DEVICE_ELEMENT {
		BEAMBLOCK,
		OBJECTIVE,
		REFLECTOR,
		TUBELENS,
		BASEPORT,
		SIDEPORT,
		MIRROR,
		LAMP,
		COUNT
	};

	POINT2 pixToMicroMeter(POINT2);

public:
	ZeissMTB() noexcept;
	~ZeissMTB();

	void setPosition(POINT3 position);
	void setPosition(POINT2 position);
	POINT3 getPosition();
	void setDevice(com *device);

public slots:
	void init();
	void connectDevice();
	void disconnectDevice();
	void errorHandler(QSerialPort::SerialPortError error);
	void setElement(DeviceElement element, double position);
	void setPreset(ScanPreset preset);
	void getElements();
	int getBeamBlock();
	void setBeamBlock(int position);
	void setLamp(int value, bool check = false);
	double getLamp();
	void getElement(DeviceElement element);
	// sets the position relative to the home position m_homePosition
	void setPositionRelativeX(double position);
	void setPositionRelativeY(double position);
	void setPositionRelativeZ(double position);
	void setPositionInPix(POINT2);
};

#endif // ZEISSMTB_H