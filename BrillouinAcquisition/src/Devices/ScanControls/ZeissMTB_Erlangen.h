#ifndef ZEISSMTB_ERLANGEN_H
#define ZEISSMTB_ERLANGEN_H

#include <atlutil.h>
#include "ODTControl.h"

#import "MTBApi.tlb" named_guids
using namespace MTBApi;

#include "..\filtermount.h"

class ZeissMTB_Erlangen: public ODTControl {
	Q_OBJECT

public:
	ZeissMTB_Erlangen() noexcept;
	~ZeissMTB_Erlangen();

	void setPosition(POINT2 position) override;
	void setPosition(POINT3 position) override;
	POINT3 getPosition() override;

public slots:
	void init() override;
	void connectDevice() override;
	void disconnectDevice() override;
	void setElement(DeviceElement element, double position) override;
	int getElement(DeviceElement element) override;
	void getElements() override;
	void setPreset(ScanPreset preset) override;

private:
	bool setElement(IMTBChangerPtr element, int position);
	int getElement(IMTBChangerPtr element);

	int getReflector();
	void setReflector(int value, bool check = false);
	int getObjective();
	void setObjective(int value, bool check = false);
	int getSideport();
	void setSideport(int value, bool check = false);
	int getRLShutter();
	void setRLShutter(int value, bool check = false);
	void setMirror(int position);
	int getMirror();
	void setBeamBlock(int value);
	int getBeamBlock();

	/*
	 * Zeiss MTB handles
	 */
	 // MTB interface pointer to the connection class
	IMTBConnectionPtr m_MTBConnection{ nullptr };
	// MTB interface ptr to the root of the tree of devices of the microscope
	IMTBRootPtr m_Root{ nullptr };
	// my ID received from MTB
	CComBSTR m_ID = _T("");
	// MTB interface pointer to the objective
	IMTBChangerPtr m_Objective{ nullptr };
	// MTB interface pointer to the reflector
	IMTBChangerPtr m_Reflector{ nullptr };
	// MTB interface pointer to the sideport
	IMTBChangerPtr m_Sideport{ nullptr };
	// MTB interface pointer to the RL shutter
	IMTBChangerPtr m_RLShutter{ nullptr };
	// MTB interface pointer to the RL/TL switch
	IMTBChangerPtr m_RLTLSwitch{ nullptr };
	// MTB interface pointer to the focus
	IMTBContinualPtr m_ObjectiveFocus{ nullptr };
	// MTB interface pointer to the stage axis x
	IMTBContinualPtr m_stageX{ nullptr };
	// MTB interface pointer to the stage axis y
	IMTBContinualPtr m_stageY{ nullptr };

	bool m_isMTBConnected{ false };

	// moveable filter mounts
	FilterMount* m_Mirror{ nullptr };

	bool m_beamBlockOpen{ false };			// current state of the beam block

	TaskHandle DOtaskHandle_BeamBlock{ 0 };

	enum class DEVICE_ELEMENT {
		BEAMBLOCK,
		OBJECTIVE,
		REFLECTOR,
		SIDEPORT,
		RLSHUTTER,
		MIRROR,
		LEDLAMP,
		COUNT
	};
};

#endif // ZEISSMTB_ERLANGEN_H