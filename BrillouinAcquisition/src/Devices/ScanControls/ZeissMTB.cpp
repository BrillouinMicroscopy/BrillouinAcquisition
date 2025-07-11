#include "stdafx.h"
#include "ZeissMTB.h"

#include <chrono>
#include <thread>

/*
 * Public definitions
 */

ZeissMTB::ZeissMTB() noexcept {

	m_deviceElements = {
		DeviceElement { "Beam Block",	2, (int)DEVICE_ELEMENT::BEAMBLOCK, { "Close", "Open" } },
		DeviceElement { "Objective",	6, (int)DEVICE_ELEMENT::OBJECTIVE },
		DeviceElement { "Reflector",	5, (int)DEVICE_ELEMENT::REFLECTOR },
		DeviceElement { "Tubelens",	3, (int)DEVICE_ELEMENT::TUBELENS },
		DeviceElement { "Baseport",	3, (int)DEVICE_ELEMENT::BASEPORT },
		DeviceElement { "Sideport",	3, (int)DEVICE_ELEMENT::SIDEPORT },
		DeviceElement { "RL Shutter",	2, (int)DEVICE_ELEMENT::RLSHUTTER, { "Close", "Open" } },
		DeviceElement { "Mirror",		2, (int)DEVICE_ELEMENT::MIRROR },
		DeviceElement { "Hal. Lamp",	0, (int)DEVICE_ELEMENT::LAMP, DEVICE_INPUT_TYPE::SLIDER }
	};

	m_presets = {
		{ "Brillouin",		ScanPreset::SCAN_BRILLOUIN,		{ {2}, {}, {1}, {3}, {1}, {2}, {1},  {}, {} }	},	// Brillouin
		{ "Calibration",	ScanPreset::SCAN_CALIBRATION,	{ {2}, {}, {1}, {3}, {1}, {3}, {1},  {}, {} }	},	// Calibration
		{ "Brightfield",	ScanPreset::SCAN_BRIGHTFIELD,	{  {}, {}, {1}, {3}, {1}, {2}, {1}, {2}, {} }	},	// Brightfield
		{ "Eyepiece",		ScanPreset::SCAN_EYEPIECE,		{ {1}, {}, {1}, {1}, {2}, {3}, {1}, {2}, {} }	},	// Eyepiece
		{ "Fluo Blue",		ScanPreset::SCAN_EPIFLUOBLUE,	{ {1}, {}, {2}, {3},  {}, {2}, {2}, {1}, {} }	},	// Fluorescence blue
		{ "Fluo Green",		ScanPreset::SCAN_EPIFLUOGREEN,	{ {1}, {}, {3}, {3},  {}, {2}, {2}, {1}, {} }	},	// Fluorescence green
		{ "Fluo Red",		ScanPreset::SCAN_EPIFLUORED,	{ {1}, {}, {4}, {3},  {}, {2}, {2}, {1}, {} }	},	// Fluorescence red
		{ "Laser off",		ScanPreset::SCAN_LASEROFF,		{ {1}, {},  {},  {},  {},  {}, {1},  {}, {} }	}	// Laser off
	};

	m_elementPositions = std::vector<double>((int)DEVICE_ELEMENT::COUNT, -1);

	// Register capabilities
	registerCapability(Capabilities::TranslationStage);
	registerCapability(Capabilities::ScaleCalibration);

	/*
	 * Initialize the scale calibration with default values (determined for a 40x objective)
	 */
	auto scaleCalibration = ScaleCalibrationData{};
	scaleCalibration.micrometerToPixX = { -7.95, 9.05 };
	scaleCalibration.micrometerToPixY = { 9.45, 8.65 };

	try {
		ScaleCalibrationHelper::initializeCalibrationFromMicrometer(&scaleCalibration);

		setScaleCalibration(scaleCalibration);
	} catch (std::exception& e) {
	}
}

ZeissMTB::~ZeissMTB() {
	disconnectDevice();
	if (m_positionTimer) {
		m_positionTimer->stop();
		m_positionTimer->deleteLater();
	}
	if (m_elementPositionTimer) {
		m_elementPositionTimer->stop();
		m_elementPositionTimer->deleteLater();
	}
	/*
	 * Clean up Zeiss MTB handles
	 */
	CoUninitialize();
}

void ZeissMTB::setPosition(POINT2 position) {
	auto success{ false };
	if (!(m_stageX && m_stageY)) {
		return;
	}
	// We have to subtract the position of the scanner to get the position of the stage.
	auto positionStage = position - m_positionScanner;
	if (abs(m_positionStage.x - positionStage.x) > 1e-6) {
		m_positionStage.x = positionStage.x;
		success = m_stageX->SetPosition(m_positionStage.x, "�m", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
	}
	if (abs(m_positionStage.y - positionStage.y) > 1e-6) {
		m_positionStage.y = positionStage.y;
		success = m_stageY->SetPosition(m_positionStage.y, "�m", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
	}
	calculateCurrentPositionBounds(POINT3{ position.x, position.y, m_positionFocus });
	announcePositions();
}

void ZeissMTB::setPosition(POINT3 position) {
	auto success{ false };
	if (!m_ObjectiveFocus) {
		return;
	}
	// Only set position if it has changed
	if (abs(m_positionFocus - position.z) > 1e-6) {
		m_positionFocus = position.z;
		success = m_ObjectiveFocus->SetPosition(m_positionFocus, "�m", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
	}
	setPosition(POINT2{ position.x, position.y });
}

void ZeissMTB::movePosition(POINT2 distance) {
	auto success{ false };
	if (!(m_stageX && m_stageY)) {
		return;
	}
	if (abs(distance.x) > 1e-6) {
		m_positionStage.x += distance.x;
		success = m_stageX->SetPosition(distance.x, "�m", (MTBCmdSetModes)(MTBCmdSetModes::MTBCmdSetModes_Synchronous | MTBCmdSetModes::MTBCmdSetModes_Relative), 500);
	}
	if (abs(distance.y) > 1e-6) {
		m_positionStage.y += distance.y;
		success = m_stageY->SetPosition(distance.y, "�m", (MTBCmdSetModes)(MTBCmdSetModes::MTBCmdSetModes_Synchronous | MTBCmdSetModes::MTBCmdSetModes_Relative), 500);
	}
	calculateCurrentPositionBounds();
	announcePositions();
}

POINT3 ZeissMTB::getPosition(PositionType positionType) {
	// Update the positions from the hardware
	if (m_stageX && m_stageY) {
		m_positionStage.x = m_stageX->GetPosition("�m");
		m_positionStage.y = m_stageY->GetPosition("�m");
	}
	if (m_ObjectiveFocus) {
		m_positionFocus = m_ObjectiveFocus->GetPosition("�m");
	}

	// Return the current position
	return ScanControl::getPosition(positionType);
}

/*
 * Public slots
 */

void ZeissMTB::init() {
	/*
	 * Initialize Zeiss MTB handles
	 */
	CoInitialize(NULL);
	try {
		// create an instance of the connection class which can connect to the server
		m_MTBConnection = IMTBConnectionPtr(CLSID_MTBConnection);
	} catch (_com_error& e) {
	}
	if (!m_positionTimer) {
		m_positionTimer = new QTimer();
		auto connection = QWidget::connect(
			m_positionTimer,
			&QTimer::timeout,
			this,
			&ZeissMTB::announcePosition
		);
	}

	if (!m_elementPositionTimer) {
		m_elementPositionTimer = new QTimer();
		auto connection = QWidget::connect(
			m_elementPositionTimer,
			&QTimer::timeout,
			this,
			&ZeissMTB::getElements
		);
	}
	calculateHomePositionBounds();
}

void ZeissMTB::connectDevice() {
	if (!m_isConnected) {
		try {
			// Don't crash when MTB server is not running
			if (!m_MTBConnection) {
				return;
			}
			/*
			 * Connect to Zeiss MTB Server
			 */
			auto res = m_MTBConnection->Login("en", &m_ID);
			// get MTB root (forcing an internal QueryInterface() on IMTBRoot!)
			m_Root = (IUnknown*)(m_MTBConnection->GetRoot((BSTR)m_ID));
			m_isConnected = true;
			/*
			 * Get the device handles
			 */
			if (m_Root) {	// Try to get element handles
				// Objective
				m_Objective = (IMTBChangerPtr)m_Root->GetComponent("MTBObjectiveChanger");
				// Reflector
				m_Reflector = (IMTBChangerPtr)m_Root->GetComponent("MTBReflectorChanger");
				// Tubelens
				m_Tubelens = (IMTBChangerPtr)m_Root->GetComponent("MTBOptovarChanger");
				// Baseport
				m_Baseport = (IMTBChangerPtr)m_Root->GetComponent("MTBBaseportChanger");
				// Sideport
				m_Sideport = (IMTBChangerPtr)m_Root->GetComponent("MTBSideportChanger");
				// Reflected light shutter
				m_RLShutter = (IMTBChangerPtr)m_Root->GetComponent("MTBRLShutter");
				// Transmission halogen lamp mirror
				m_Mirror = (IMTBChangerPtr)m_Root->GetComponent("MTBTLLampChanger");
				// Transmission halogen lamp
				m_Lamp = (IMTBContinualPtr)m_Root->GetComponent("MTBTLHalogenLamp");
				// Objective focus
				m_ObjectiveFocus = (IMTBContinualPtr)m_Root->GetComponent("MTBFocus");
				// Stage axis x
				m_stageX = (IMTBContinualPtr)m_Root->GetComponent("MTBStageAxisX");
				// Stage axis y
				m_stageY = (IMTBContinualPtr)m_Root->GetComponent("MTBStageAxisY");
			}

			if (Thorlabs_FF::TLI_BuildDeviceList() == 0) {
				auto ret = Thorlabs_FF::FF_Open(m_serialNo_FF2);
				if (ret == 0) {
					Thorlabs_FF::FF_StartPolling(m_serialNo_FF2, 200);
				}
			}

			if (m_isConnected) {
				setPreset(ScanPreset::SCAN_BRILLOUIN);
				getElements();
				m_homePosition = getPosition();
				startAnnouncingPosition();
				startAnnouncingElementPosition();
				calculateHomePositionBounds();
				calculateCurrentPositionBounds();
			}

		} catch (QString& e) {
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
			} catch (_com_error& e) {
			}

			m_MTBConnection->Close();
			m_isConnected = false;
		}
	}
	emit(connectedDevice(m_isConnected));
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

int ZeissMTB::getElement(const DeviceElement& element) {
	return -1;
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

/*
 * Private definitions
 */

bool ZeissMTB::setElement(IMTBChangerPtr element, int position) {
	if (!element) {
		return false;
	}
	return element->SetPosition(position, MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
}

int ZeissMTB::getElement(IMTBChangerPtr element) {
	if (!element) {
		return -1;
	}
	return element->GetPosition();
}

void ZeissMTB::setBeamBlock(int position) {
	Thorlabs_FF::FF_MoveToPosition(m_serialNo_FF2, (Thorlabs_FF::FF_Positions)position);
	auto i{ 0 };
	while (getBeamBlock() != position && i++ < 10) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

int ZeissMTB::getBeamBlock() {
	return Thorlabs_FF::FF_GetPosition(m_serialNo_FF2);
}

void ZeissMTB::setReflector(int position, bool block) {
	// Set the position
	auto success = setElement(m_Reflector, position);
}

int ZeissMTB::getReflector() {
	return getElement(m_Reflector);
}

void ZeissMTB::setObjective(int position, bool block) {
	// Set the position
	auto success = setElement(m_Objective, position);
}

int ZeissMTB::getObjective() {
	return getElement(m_Objective);
}

void ZeissMTB::setTubelens(int position, bool block) {
	// Set the position
	auto success = setElement(m_Tubelens, position);
}

int ZeissMTB::getTubelens() {
	return getElement(m_Tubelens);
}

void ZeissMTB::setBaseport(int position, bool block) {
	// Set the position
	auto success = setElement(m_Baseport, position);
}

int ZeissMTB::getBaseport() {
	return getElement(m_Baseport);
}

void ZeissMTB::setSideport(int position, bool block) {
	// Set the position
	auto success = setElement(m_Sideport, position);
}

int ZeissMTB::getSideport() {
	return getElement(m_Sideport);
}

void ZeissMTB::setRLShutter(int position, bool block) {
	// Set the position
	auto success = setElement(m_RLShutter, position);
}

int ZeissMTB::getRLShutter() {
	return getElement(m_RLShutter);
}

void ZeissMTB::setMirror(int position, bool block) {
	// Set the position
	auto success = setElement(m_Mirror, position);
}

int ZeissMTB::getMirror() {
	return getElement(m_Mirror);
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

double ZeissMTB::getLamp() {
	if (!m_Lamp) {
		return 0.0;
	}
	return m_Lamp->GetPosition("%");
}