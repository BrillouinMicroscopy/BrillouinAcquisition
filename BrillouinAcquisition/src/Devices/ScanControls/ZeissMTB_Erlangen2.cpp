#include "stdafx.h"
#include "ZeissMTB_Erlangen2.h"

/*
 * Public definitions
 */

ZeissMTB_Erlangen2::ZeissMTB_Erlangen2() noexcept {

	m_deviceElements = {
		DeviceElement { "Objective",			6, (int)DEVICE_ELEMENT::OBJECTIVE },
		DeviceElement { "Reflector",			4, (int)DEVICE_ELEMENT::REFLECTOR,	{ "IR", "Green", "Red", "Blue" } },
		DeviceElement { "Sideport",			3, (int)DEVICE_ELEMENT::SIDEPORT,	{ "Eyepiece", "Left", "Right" } },
		DeviceElement { "RL Shutter",			2, (int)DEVICE_ELEMENT::RLSHUTTER,	{ "Close", "Open" } },
		DeviceElement { "Mirror",				2, (int)DEVICE_ELEMENT::MIRROR,		{ "Brillouin", "Calibration"}},
		DeviceElement { "LED illumination",	2, (int)DEVICE_ELEMENT::LEDLAMP,	{ "Off", "On" } }
	};

	m_presets = {
		{ "Brillouin",		ScanPreset::SCAN_BRILLOUIN,		{{}, {1}, {3}, {2}, {1}, {1}	}},	// Brillouin
		{ "Calibration",	ScanPreset::SCAN_CALIBRATION,	{{}, {1}, {3}, {1}, {2}, {1}	}},	// Calibration
		{ "Confocal",		ScanPreset::SCAN_CONFOCAL,		{{}, {1}, {2}, {1}, {2}, {1}	}},	// Confocal
		{ "Brightfield",	ScanPreset::SCAN_BRIGHTFIELD,	{{}, {1}, {3}, {2},  {}, {2}	}},	// Brightfield
		{ "Eyepiece",		ScanPreset::SCAN_EYEPIECE,		{{}, {1}, {1}, {1}, {1}, {1}	}},	// Eyepiece
	};

	m_elementPositions = std::vector<double>((int)DEVICE_ELEMENT::COUNT, -1);

	// Register capabilities
	registerCapability(Capabilities::TranslationStage);
	registerCapability(Capabilities::ScaleCalibration);

	/*
	 * Initialize the scale calibration with default values (determined for a 20x objective)
	 */
	auto scale = double{ 0.235674 };					// [�m/pix] image scale

	auto scaleCalibration = ScaleCalibrationData{};
	scaleCalibration.pixToMicrometerX = { 0, scale };	// camera x axis is stage y axis
	scaleCalibration.pixToMicrometerY = { scale, 0 };	// camera y axis is state x axis

	try {
		ScaleCalibrationHelper::initializeCalibrationFromPixel(&scaleCalibration);

		setScaleCalibration(scaleCalibration);
	} catch (std::exception& e) {
	}
}

ZeissMTB_Erlangen2::~ZeissMTB_Erlangen2() {
	disconnectDevice();
	if (m_positionTimer) {
		m_positionTimer->stop();
		m_positionTimer->deleteLater();
	}
	if (m_elementPositionTimer) {
		m_elementPositionTimer->stop();
		m_elementPositionTimer->deleteLater();
	}
	if (m_Mirror) {
		m_Mirror->deleteLater();
	}
	/*
	 * Clean up Zeiss MTB handles
	 */
	CoUninitialize();
}

void ZeissMTB_Erlangen2::setPosition(POINT2 position) {
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

void ZeissMTB_Erlangen2::setPosition(POINT3 position) {
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

void ZeissMTB_Erlangen2::movePosition(POINT2 distance) {
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

POINT3 ZeissMTB_Erlangen2::getPosition(PositionType positionType) {
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

void ZeissMTB_Erlangen2::init() {
	/*
	 * Initialize Zeiss MTB handles
	 */
	CoInitialize(NULL);
	try {
		// create an instance of the connection class which can connect to the server
		m_MTBConnection = IMTBConnectionPtr(CLSID_MTBConnection);
	} catch (_com_error& e) {
	}

	if (!m_Mirror) {
		m_Mirror = new FilterMount("COM7");
		m_Mirror->init();
	}

	if (!m_positionTimer) {
		m_positionTimer = new QTimer();
		auto connection = QWidget::connect(
			m_positionTimer,
			&QTimer::timeout,
			this,
			&ZeissMTB_Erlangen2::announcePosition
		);
	}

	if (!m_elementPositionTimer) {
		m_elementPositionTimer = new QTimer();
		auto connection = QWidget::connect(
			m_elementPositionTimer,
			&QTimer::timeout,
			this,
			&ZeissMTB_Erlangen2::getElements
		);
	}
	calculateHomePositionBounds();
}

void ZeissMTB_Erlangen2::connectDevice() {
	if (!m_isConnected) {
		try {
			// Don't crash when MTB server is not running
			if (!m_MTBConnection) {
				return;
			}
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
			if (m_Root) {	// Try to get element handles
				// Objective
				m_Objective = (IMTBChangerPtr)m_Root->GetComponent("MTBObjectiveChanger");
				// Reflector
				m_Reflector = (IMTBChangerPtr)m_Root->GetComponent("MTBReflectorChanger");
				// Sideport
				m_Sideport = (IMTBChangerPtr)m_Root->GetComponent("MTBSideportChanger");
				// Reflected light shutter
				m_RLShutter = (IMTBChangerPtr)m_Root->GetComponent("MTBRLShutter");
				// Reflected light / transmitted light switch
				m_TLShutter = (IMTBChangerPtr)m_Root->GetComponent("MTBTLShutter");
				// Reflected light / transmitted light switch
				m_RLTLSwitch = (IMTBChangerPtr)m_Root->GetComponent("MTBRLTLSwitch");
				// Objective focus
				m_ObjectiveFocus = (IMTBContinualPtr)m_Root->GetComponent("MTBFocus");
				// Stage axis x
				m_stageX = (IMTBContinualPtr)m_Root->GetComponent("MTBStageAxisX");
				// Stage axis y
				m_stageY = (IMTBContinualPtr)m_Root->GetComponent("MTBStageAxisY");
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

		// Connect filter mounts
		m_Mirror->connectDevice();
	}
	emit(connectedDevice(m_isConnected));
}

void ZeissMTB_Erlangen2::disconnectDevice() {
	if (m_isConnected) {
		stopAnnouncingPosition();
		stopAnnouncingElementPosition();

		if (m_MTBConnection != NULL && m_ID != "") {
			// logout from MTB
			try {
				m_MTBConnection->Logout((BSTR)m_ID);
			} catch (_com_error& e) {
			}

			m_MTBConnection->Close();
			m_isConnected = false;
		}

		// Disconnect filter mounts
		m_Mirror->disconnectDevice();
	}
	emit(connectedDevice(m_isConnected));
}

void ZeissMTB_Erlangen2::setElement(DeviceElement element, double position) {
	switch ((DEVICE_ELEMENT)element.index) {
		case DEVICE_ELEMENT::REFLECTOR:
			setReflector((int)position, true);
			break;
		case DEVICE_ELEMENT::OBJECTIVE:
			setObjective((int)position, true);
			break;
		case DEVICE_ELEMENT::SIDEPORT:
			setSideport((int)position, true);
			break;
		case DEVICE_ELEMENT::RLSHUTTER:
			setRLShutter((int)position, true);
			break;
		case DEVICE_ELEMENT::MIRROR:
			setMirror((int)position);
			break;
		case DEVICE_ELEMENT::LEDLAMP:
			//setLEDLamp((int)position - 1);
			setTLShutter((int)position, true);
			break;
		default:
			break;
	}
	m_elementPositions[element.index] = position;
	checkPresets();
	emit(elementPositionChanged(element, position));
}

int ZeissMTB_Erlangen2::getElement(const DeviceElement& element) {
	return -1;
}

void ZeissMTB_Erlangen2::getElements() {
	m_elementPositionsTmp = m_elementPositions;
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::REFLECTOR] = getReflector();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::OBJECTIVE] = getObjective();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::SIDEPORT] = getSideport();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::RLSHUTTER] = getRLShutter();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::LEDLAMP] = getTLShutter();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::MIRROR] = getMirror();
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

bool ZeissMTB_Erlangen2::setElement(IMTBChangerPtr element, int position) {
	if (!element) {
		return false;
	}
	return element->SetPosition(position, MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
}

int ZeissMTB_Erlangen2::getElement(IMTBChangerPtr element) {
	if (!element) {
		return -1;
	}
	return element->GetPosition();
}

void ZeissMTB_Erlangen2::setReflector(int position, bool block) {
	// Set the position
	auto success = setElement(m_Reflector, position);
}

int ZeissMTB_Erlangen2::getReflector() {
	return getElement(m_Reflector);
}

void ZeissMTB_Erlangen2::setObjective(int position, bool block) {
	// Set the position
	auto success = setElement(m_Objective, position);
}

void ZeissMTB_Erlangen2::setSideport(int position, bool block) {
	// Set the position
	auto success = setElement(m_Sideport, position);
}

int ZeissMTB_Erlangen2::getObjective() {
	return getElement(m_Objective);
}

int ZeissMTB_Erlangen2::getSideport() {
	return getElement(m_Sideport);
}

void ZeissMTB_Erlangen2::setRLShutter(int position, bool block) {
	// Set the position
	auto success = setElement(m_RLShutter, position);
}

int ZeissMTB_Erlangen2::getRLShutter() {
	return getElement(m_RLShutter);
}

void ZeissMTB_Erlangen2::setTLShutter(int position, bool block) {
	// Set the position
	auto success = setElement(m_TLShutter, position);
}

int ZeissMTB_Erlangen2::getTLShutter() {
	return getElement(m_TLShutter);
}

void ZeissMTB_Erlangen2::setMirror(int position) {
	// calculate the position to set, slots are spaced every 32 mm
	auto pos = 60.0 * ((double)position - 1);
	// This device has 1024 encoder pulses per mm
	pos *= 1024;
	m_Mirror->setPosition(pos);
}

int ZeissMTB_Erlangen2::getMirror() {
	auto pos = m_Mirror->getPosition();
	// This device has 1024 encoder pulses per mm
	pos /= 1024;
	// Somehow the filter mount does not position the filters very accurately.
	// It can be off by multiple millimeters and the error increases with positions farther away.
	// E.g. requested 0 -> got 0, 32 -> 31, 64 -> 62, 96 -> 93
	for (gsl::index position{ 0 }; position < 2; position++) {
		if (abs(pos - 60.0 * position) < (1.0 + position)) {
			return (position + 1);
		}
	}
	return -1;
}