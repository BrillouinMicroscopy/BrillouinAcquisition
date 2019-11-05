#include "stdafx.h"
#include "ZeissMTB.h"

POINT2 ZeissMTB::pixToMicroMeter(POINT2) {
	return POINT2();
}

ZeissMTB::ZeissMTB() noexcept {

	m_deviceElements = {
		{ "Beam Block",	2, (int)DEVICE_ELEMENT::BEAMBLOCK, { "Close", "Open" } },
		{ "Objective",	6, (int)DEVICE_ELEMENT::OBJECTIVE },
		{ "Reflector",	5, (int)DEVICE_ELEMENT::REFLECTOR },
		{ "Tubelens",	3, (int)DEVICE_ELEMENT::TUBELENS },
		{ "Baseport",	3, (int)DEVICE_ELEMENT::BASEPORT },
		{ "Sideport",	3, (int)DEVICE_ELEMENT::SIDEPORT },
		{ "RL Shutter",	2, (int)DEVICE_ELEMENT::RLSHUTTER, { "Close", "Open" } },
		{ "Mirror",		2, (int)DEVICE_ELEMENT::MIRROR },
		{ "Hal. Lamp",	0, (int)DEVICE_ELEMENT::LAMP, DEVICE_INPUT_TYPE::SLIDER }
	};

	m_presets = {
		{ "Brillouin",		ScanPreset::SCAN_BRILLOUIN,		{ {2}, {}, {1}, {3}, {1}, {2}, {1},  {}, {} } },	// Brillouin
		{ "Calibration",	ScanPreset::SCAN_CALIBRATION,	{ {2}, {}, {1}, {3}, {1}, {3}, {1},  {}, {} } },	// Calibration
		{ "Brightfield",	ScanPreset::SCAN_BRIGHTFIELD,	{ {2}, {}, {1}, {3}, {1}, {2}, {1}, {2}, {} } },	// Brightfield
		{ "Eyepiece",		ScanPreset::SCAN_EYEPIECE,		{ {2}, {}, {1}, {3}, {2}, {3}, {1}, {2}, {} } },	// Eyepiece
		{ "Fluo Blue",		ScanPreset::SCAN_EPIFLUOBLUE,	{ {1}, {}, {2}, {3},  {}, {2}, {2}, {1}, {} } },	// Fluorescence blue
		{ "Fluo Green",		ScanPreset::SCAN_EPIFLUOGREEN,	{ {1}, {}, {3}, {3},  {}, {2}, {2}, {1}, {} } },	// Fluorescence green
		{ "Fluo Red",		ScanPreset::SCAN_EPIFLUORED,	{ {1}, {}, {4}, {3},  {}, {2}, {2}, {1}, {} } },	// Fluorescence red
		{ "Laser off",		ScanPreset::SCAN_LASEROFF,		{ {1}, {},  {},  {},  {},  {}, {1},  {}, {} } }		// Laser off
	};

	// bounds of the stage
	m_absoluteBounds = {
		-150000,	// [µm] minimal x-value
		 150000,	// [µm] maximal x-value
		-150000,	// [µm] minimal y-value
		 150000,	// [µm] maximal y-value
		-150000,	// [µm] minimal z-value
		 150000		// [µm] maximal z-value
	};

	m_elementPositions = std::vector<double>((int)DEVICE_ELEMENT::COUNT, -1);
}

ZeissMTB::~ZeissMTB() {
	positionTimer->stop();
	elementPositionTimer->stop();
	disconnectDevice();
	/*
	 * Clean up Zeiss MTB handles
	 */
	CoUninitialize();
}

void ZeissMTB::init() {
	/*
	 * Initialize Zeiss MTB handles
	 */
	CoInitialize(NULL);
	try {
		// create an instance of the connection class which can connect to the server
		m_MTBConnection = IMTBConnectionPtr(CLSID_MTBConnection);
	} catch (_com_error e) {
	}

	positionTimer = new QTimer();
	QMetaObject::Connection connection = QWidget::connect(positionTimer, SIGNAL(timeout()), this, SLOT(announcePosition()));

	elementPositionTimer = new QTimer();
	connection = QWidget::connect(elementPositionTimer, SIGNAL(timeout()), this, SLOT(getElements()));
	calculateHomePositionBounds();
}

void ZeissMTB::connectDevice() {
	if (!m_isConnected) {
		try {
			/*
			 * Connect to Zeiss MTB Server
			 */
			m_MTBConnection->Login("en", &m_ID);
			// get MTB root (forcing an internal QueryInterface() on IMTBRoot!)
			m_Root = (IUnknown*)(m_MTBConnection->GetRoot((BSTR)m_ID));
			m_isConnected = true;
			/*
			 * Get the device handles
			 */
			m_Stand = (IUnknown*)(m_Root->GetDevice(0));	// Stand handle
			if (m_Stand) {	// Try to get element handles
				// Objective
				m_Objective = (IMTBChangerPtr)m_Stand->GetComponent("MTBObjectiveChanger");
				// Reflector
				m_Reflector = (IMTBChangerPtr)m_Stand->GetComponent("MTBReflectorChanger");
				// Tubelens
				m_Tubelens = (IMTBChangerPtr)m_Stand->GetComponent("MTBOptovarChanger");
				// Baseport
				m_Baseport = (IMTBChangerPtr)m_Stand->GetComponent("MTBBaseportChanger");
				// Sideport
				m_Sideport = (IMTBChangerPtr)m_Stand->GetComponent("MTBSideportChanger");
				// Reflected light shutter
				m_RLShutter = (IMTBChangerPtr)m_Stand->GetComponent("MTBRLShutter");
				// Transmission halogen lamp mirror
				m_Mirror = (IMTBChangerPtr)m_Stand->GetComponent("MTBTLLampChanger");
				// Transmission halogen lamp
				m_Lamp = (IMTBContinualPtr)m_Stand->GetComponent("MTBTLHalogenLamp");
			}
			m_Focus = (IUnknown*)(m_Root->GetDevice(1));	// Stand handle
			if (m_Focus) {
				// Objective focus
				m_ObjectiveFocus = (IMTBContinualPtr)m_Focus->GetComponent("MTBFocus");
			}
			m_MCU = (IUnknown*)(m_Root->GetDevice(2));	// Stand handle
			if (m_MCU) {
				// Stage axis x
				m_stageX = (IMTBContinualPtr)m_MCU->GetComponent("MTBStageAxisX");
				// Stage axis y
				m_stageY = (IMTBContinualPtr)m_MCU->GetComponent("MTBStageAxisY");
			}

			Thorlabs_FF::FF_Open(m_serialNo_FF2);
			Thorlabs_FF::FF_StartPolling(m_serialNo_FF2, 200);

			if (m_isConnected) {
				setPreset(ScanPreset::SCAN_BRILLOUIN);
				getElements();
				m_homePosition = getPosition();
				startAnnouncingPosition();
				startAnnouncingElementPosition();
				calculateHomePositionBounds();
				calculateCurrentPositionBounds();
			}

		} catch (QString e) {
			// todo
		}
	}
	emit(connectedDevice(m_isConnected));
}

void ZeissMTB::disconnectDevice() {
	if (m_isConnected) {
		stopAnnouncingPosition();
		stopAnnouncingElementPosition();
		Thorlabs_FF::FF_Close(m_serialNo_FF2);
		Thorlabs_FF::FF_StopPolling(m_serialNo_FF2);

		if (m_MTBConnection != NULL && m_ID != "") {
			// logout from MTB
			try {
				m_MTBConnection->Logout((BSTR)m_ID);
			} catch (_com_error e) {
			}

			m_MTBConnection->Close();
			m_isConnected = false;
		}
	}
	emit(connectedDevice(m_isConnected));
}

void ZeissMTB::setPosition(POINT3 position) {
	bool success{ false };
	if (m_MCU) {
		success = m_stageX->SetPosition(position.x, "µm", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
		success = m_stageY->SetPosition(position.y, "µm", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
	}
	if (m_ObjectiveFocus) {
		success = m_ObjectiveFocus->SetPosition(position.z, "µm", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
	}
	calculateCurrentPositionBounds(position);
}

void ZeissMTB::setPosition(POINT2 position) {
	bool success{ false };
	if (m_MCU) {
		success = m_stageX->SetPosition(position.x, "µm", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
		success = m_stageY->SetPosition(position.y, "µm", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
	}
	calculateCurrentPositionBounds();
}

void ZeissMTB::setPositionRelativeX(double positionX) {
	bool success{ false };
	if (m_MCU) {
		success = m_stageX->SetPosition(positionX + m_homePosition.x, "µm", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
	}
	calculateCurrentPositionBounds();
}

void ZeissMTB::setPositionRelativeY(double positionY) {
	bool success{ false };
	if (m_MCU) {
		success = m_stageY->SetPosition(positionY + m_homePosition.y, "µm", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
	}
	calculateCurrentPositionBounds();
}

void ZeissMTB::setPositionRelativeZ(double positionZ) {
	bool success{ false };
	if (m_ObjectiveFocus) {
		success = m_ObjectiveFocus->SetPosition(positionZ + m_homePosition.z, "µm", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
	}
	calculateCurrentPositionBounds();
}

void ZeissMTB::setPositionInPix(POINT2) {
	// Does nothing for now, since for the 780 nm setup no spatial calibration is in place yet.
}

POINT3 ZeissMTB::getPosition() {
	double x{ 0 };
	double y{ 0 };
	double z{ 0 };
	if (m_MCU) {
		x = m_stageX->GetPosition("µm");
		y = m_stageY->GetPosition("µm");
	}
	if (m_ObjectiveFocus) {
		z = m_ObjectiveFocus->GetPosition("µm");
	}
	return POINT3{ x, y, z };
}

void ZeissMTB::setPreset(ScanPreset presetType) {
	auto preset = getPreset(presetType);
	getElements();

	for (gsl::index ii = 0; ii < m_deviceElements.size(); ii++) {
		// check if element position needs to be changed
		if (!preset.elementPositions[ii].empty() && !simplemath::contains(preset.elementPositions[ii], m_elementPositions[ii])) {
			setElement(m_deviceElements[ii], preset.elementPositions[ii][0]);
			m_elementPositions[ii] = preset.elementPositions[ii][0];
		}
	}
	checkPresets();
	emit(elementPositionsChanged(m_elementPositions));
}

int ZeissMTB::getElement(DeviceElement element) {
	return -1;
}

void ZeissMTB::setElement(DeviceElement element, double position) {
	switch ((DEVICE_ELEMENT)element.index) {
		case DEVICE_ELEMENT::BEAMBLOCK:
			setBeamBlock((int)position);
			break;
		case DEVICE_ELEMENT::REFLECTOR:
			setReflector((int)position, true);
			break;
		case DEVICE_ELEMENT::OBJECTIVE:
			setObjective((int)position, true);
			break;
		case DEVICE_ELEMENT::TUBELENS:
			setTubelens((int)position, true);
			break;
		case DEVICE_ELEMENT::BASEPORT:
			setBaseport((int)position, true);
			break;
		case DEVICE_ELEMENT::SIDEPORT:
			setSideport((int)position, true);
			break;
		case DEVICE_ELEMENT::RLSHUTTER:
			setRLShutter((int)position, true);
			break;
		case DEVICE_ELEMENT::MIRROR:
			setMirror((int)position, true);
			break;
		case DEVICE_ELEMENT::LAMP:
			setLamp(position, true);
			break;
		default:
			break;
	}
	m_elementPositions[element.index] = position;
	checkPresets();
	emit(elementPositionChanged(element, position));
}

int ZeissMTB::getElement(IMTBChangerPtr element) {
	if (!element) {
		return -1;
	}
	return element->GetPosition();
}

bool ZeissMTB::setElement(IMTBChangerPtr element, int position) {
	if (!element) {
		return false;
	}
	return element->SetPosition(position, MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
}

void ZeissMTB::getElements() {
	m_elementPositionsTmp = m_elementPositions;
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::BEAMBLOCK] = getBeamBlock();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::REFLECTOR] = getReflector();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::OBJECTIVE] = getObjective();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::TUBELENS] = getTubelens();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::BASEPORT] = getBaseport();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::SIDEPORT] = getSideport();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::RLSHUTTER] = getRLShutter();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::MIRROR] = getMirror();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::LAMP] = getLamp();
	// We only emit changed positions
	if (m_elementPositionsTmp != m_elementPositions) {
		m_elementPositions = m_elementPositionsTmp;
		checkPresets();
		emit(elementPositionsChanged(m_elementPositions));
	}
}

int ZeissMTB::getBeamBlock() {
	return Thorlabs_FF::FF_GetPosition(m_serialNo_FF2);
}

void ZeissMTB::setBeamBlock(int position) {
	Thorlabs_FF::FF_MoveToPosition(m_serialNo_FF2, (Thorlabs_FF::FF_Positions)position);
	auto i{ 0 };
	while (getBeamBlock() != position && i++ < 10) {
		Sleep(100);
	}
}

int ZeissMTB::getReflector() {
	return getElement(m_Reflector);
}

void ZeissMTB::setReflector(int position, bool block) {
	// Set the position
	auto success = setElement(m_Reflector, position);
}

int ZeissMTB::getObjective() {
	return getElement(m_Objective);
}

void ZeissMTB::setObjective(int position, bool block) {
	// Set the position
	auto success = setElement(m_Objective, position);
}

int ZeissMTB::getTubelens() {
	return getElement(m_Tubelens);
}

void ZeissMTB::setTubelens(int position, bool block) {
	// Set the position
	auto success = setElement(m_Tubelens, position);
}

int ZeissMTB::getBaseport() {
	return getElement(m_Baseport);
}

void ZeissMTB::setBaseport(int position, bool block) {
	// Set the position
	auto success = setElement(m_Baseport, position);
}

int ZeissMTB::getSideport() {
	return getElement(m_Sideport);
}

void ZeissMTB::setSideport(int position, bool block) {
	// Set the position
	auto success = setElement(m_Sideport, position);
}

int ZeissMTB::getRLShutter() {
	return getElement(m_RLShutter);
}

void ZeissMTB::setRLShutter(int position, bool block) {
	// Set the position
	auto success = setElement(m_RLShutter, position);
}

int ZeissMTB::getMirror() {
	return getElement(m_Mirror);
}

void ZeissMTB::setMirror(int position, bool block) {
	// Set the position
	auto success = setElement(m_Mirror, position);
}

double ZeissMTB::getLamp() {
	if (!m_Lamp) {
		return 0.0;
	}
	return m_Lamp->GetPosition("%");
}

void ZeissMTB::setLamp(int voltage, bool block) {
	if (!m_Lamp) {
		return;
	}
	if (voltage > 100) voltage = 100;
	if (voltage < 0) voltage = 0;
	// Set the voltage
	auto success = m_Lamp->SetPosition(voltage, "%", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
}