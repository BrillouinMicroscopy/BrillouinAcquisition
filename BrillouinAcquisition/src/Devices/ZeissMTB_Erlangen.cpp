#include "stdafx.h"
#include "ZeissMTB_Erlangen.h"

/*
 * Public definitions
 */

ZeissMTB_Erlangen::ZeissMTB_Erlangen() noexcept {

	m_deviceElements = {
		{ "Beam Block",			2, (int)DEVICE_ELEMENT::BEAMBLOCK,	{ "Close", "Open" } },
		{ "Objective",			6, (int)DEVICE_ELEMENT::OBJECTIVE },
		{ "Reflector",			6, (int)DEVICE_ELEMENT::REFLECTOR,	{ "Green", "Red", "Blue" } },
		{ "Sideport",			3, (int)DEVICE_ELEMENT::SIDEPORT,	{ "Eyepiece", "Left", "Right" } },
		{ "RL Shutter",			2, (int)DEVICE_ELEMENT::RLSHUTTER,	{ "Close", "Open" } },
		{ "Mirror",				2, (int)DEVICE_ELEMENT::MIRROR,		{ "ODT", "Brillouin" } },
		{ "LED illumination",	2, (int)DEVICE_ELEMENT::LEDLAMP,	{ "Off", "On" } }
	};

	m_presets = {
		{ "Brillouin",		ScanPreset::SCAN_BRILLOUIN,		{ {2}, {}, {4}, {2}, {1}, {2}, {2} } },	// Brillouin
		{ "Calibration",	ScanPreset::SCAN_CALIBRATION,	{ {2}, {}, {4}, {2}, {1}, {1}, {2} } },	// Calibration
		{ "ODT",			ScanPreset::SCAN_ODT,			{ {2}, {}, {4}, {2}, {1}, {1}, {1} } },	// ODT
		{ "Brightfield",	ScanPreset::SCAN_BRIGHTFIELD,	{ {2}, {}, {4}, {2}, {1}, {2}, {2} } },	// Brightfield
		{ "Eyepiece",		ScanPreset::SCAN_EYEPIECE,		{ {2}, {}, {4}, {1}, {1}, {2}, {2} } },	// Eyepiece
		{ "Fluo Blue",		ScanPreset::SCAN_EPIFLUOBLUE,	{ {1}, {}, {3},  {}, {2}, {1}, {1} } },	// Fluorescence blue
		{ "Fluo Green",		ScanPreset::SCAN_EPIFLUOGREEN,	{ {1}, {}, {1},  {}, {2}, {1}, {1} } },	// Fluorescence green
		{ "Fluo Red",		ScanPreset::SCAN_EPIFLUORED,	{ {1}, {}, {2},  {}, {2}, {1}, {1} } }	// Fluorescence red
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

ZeissMTB_Erlangen::~ZeissMTB_Erlangen() {
	positionTimer->stop();
	elementPositionTimer->stop();
	disconnectDevice();
	/*
	 * Clean up Zeiss MTB handles
	 */
	CoUninitialize();
}

void ZeissMTB_Erlangen::setPosition(POINT2 position) {
	bool success{ false };
	if (m_stageX && m_stageY) {
		success = m_stageX->SetPosition(position.x, "µm", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
		success = m_stageY->SetPosition(position.y, "µm", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
	}
	calculateCurrentPositionBounds();
}

void ZeissMTB_Erlangen::setPosition(POINT3 position) {
	bool success{ false };
	if (m_stageX && m_stageY) {
		success = m_stageX->SetPosition(position.x, "µm", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
		success = m_stageY->SetPosition(position.y, "µm", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
	}
	if (m_ObjectiveFocus) {
		success = m_ObjectiveFocus->SetPosition(position.z, "µm", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
	}
	calculateCurrentPositionBounds(position);
}

POINT3 ZeissMTB_Erlangen::getPosition() {
	double x{ 0 };
	double y{ 0 };
	double z{ 0 };
	if (m_stageX && m_stageY) {
		x = m_stageX->GetPosition("µm");
		y = m_stageY->GetPosition("µm");
	}
	if (m_ObjectiveFocus) {
		z = m_ObjectiveFocus->GetPosition("µm");
	}
	return POINT3{ x, y, z };
}

/*
 * Public slots
 */

void ZeissMTB_Erlangen::init() {
	/*
	 * Initialize Zeiss MTB handles
	 */
	CoInitialize(NULL);
	try {
		// create an instance of the connection class which can connect to the server
		m_MTBConnection = IMTBConnectionPtr(CLSID_MTBConnection);
	} catch (_com_error e) {
	}

	m_Mirror = new FilterMount("COM5");
	m_Mirror->init();

	positionTimer = new QTimer();
	QMetaObject::Connection connection = QWidget::connect(
		positionTimer,
		&QTimer::timeout,
		this,
		&ZeissMTB_Erlangen::announcePosition
	);

	elementPositionTimer = new QTimer();
	connection = QWidget::connect(
		elementPositionTimer,
		&QTimer::timeout,
		this,
		&ZeissMTB_Erlangen::getElements
	);
	calculateHomePositionBounds();
}

void ZeissMTB_Erlangen::connectDevice() {
	if (!m_isConnected) {
		// Connect NIDAQ board
		ODTControl::connectDevice();

		// Create task for digital output to beam block
		DAQmxCreateTask("DO_BEAMBLOCK", &DOtaskHandle_BeamBlock);
		// Configure digital output channel
		DAQmxCreateDOChan(DOtaskHandle_BeamBlock, "Dev1/Port0/Line3:3", "DO_BEAMBLOCK", DAQmx_Val_ChanForAllLines);
		// Configure regen mode
		DAQmxSetWriteAttribute(DOtaskHandle_BeamBlock, DAQmx_Write_RegenMode, DAQmx_Val_AllowRegen);
		// Start digital task
		DAQmxStartTask(DOtaskHandle_BeamBlock);
		// Set digital line to low
		DAQmxWriteDigitalLines(DOtaskHandle_BeamBlock, 1, false, 10, DAQmx_Val_GroupByChannel, &m_TTL.low, NULL, NULL);

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

		} catch (QString e) {
			// todo
		}

		// Connect filter mounts
		m_Mirror->connectDevice();
	}
	emit(connectedDevice(m_isConnected));
}

void ZeissMTB_Erlangen::disconnectDevice() {
	if (m_isConnected) {
		stopAnnouncingPosition();
		stopAnnouncingElementPosition();
		ODTControl::disconnectDevice();

		if (m_MTBConnection != NULL && m_ID != "") {
			// logout from MTB
			try {
				m_MTBConnection->Logout((BSTR)m_ID);
			} catch (_com_error e) {
			}

			m_MTBConnection->Close();
			m_isConnected = false;
		}

		// Disconnect filter mounts
		m_Mirror->disconnectDevice();
	}
	emit(connectedDevice(m_isConnected));
}

void ZeissMTB_Erlangen::setElement(DeviceElement element, double position) {
	switch ((DEVICE_ELEMENT)element.index) {
	case DEVICE_ELEMENT::BEAMBLOCK:
		setBeamBlock((int)position - 1);
		break;
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
		setLEDLamp((int)position - 1);
		break;
	default:
		break;
	}
	m_elementPositions[element.index] = position;
	checkPresets();
	emit(elementPositionChanged(element, position));
}

int ZeissMTB_Erlangen::getElement(DeviceElement element) {
	return -1;
}

void ZeissMTB_Erlangen::getElements() {
	m_elementPositionsTmp = m_elementPositions;
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::REFLECTOR] = getReflector();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::OBJECTIVE] = getObjective();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::SIDEPORT] = getSideport();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::RLSHUTTER] = getRLShutter();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::MIRROR] = getMirror();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::BEAMBLOCK] = (double)getBeamBlock() + 1;
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::LEDLAMP] = (double)getLEDLamp() + 1;
	// We only emit changed positions
	if (m_elementPositionsTmp != m_elementPositions) {
		m_elementPositions = m_elementPositionsTmp;
		checkPresets();
		emit(elementPositionsChanged(m_elementPositions));
	}
}

void ZeissMTB_Erlangen::setPreset(ScanPreset presetType) {
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

void ZeissMTB_Erlangen::setPositionRelativeX(double positionX) {
	bool success{ false };
	if (m_stageX) {
		success = m_stageX->SetPosition(positionX + m_homePosition.x, "µm", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
	}
	calculateCurrentPositionBounds();
}

void ZeissMTB_Erlangen::setPositionRelativeY(double positionY) {
	bool success{ false };
	if (m_stageY) {
		success = m_stageY->SetPosition(positionY + m_homePosition.y, "µm", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
	}
	calculateCurrentPositionBounds();
}

void ZeissMTB_Erlangen::setPositionRelativeZ(double positionZ) {
	bool success{ false };
	if (m_ObjectiveFocus) {
		success = m_ObjectiveFocus->SetPosition(positionZ + m_homePosition.z, "µm", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
	}
	calculateCurrentPositionBounds();
}

void ZeissMTB_Erlangen::setPositionInPix(POINT2) {
	// Does nothing for now, since for the 780 nm setup no spatial calibration is in place yet.
}

/*
 * Private definitions
 */

POINT2 ZeissMTB_Erlangen::pixToMicroMeter(POINT2) {
	return POINT2();
}

bool ZeissMTB_Erlangen::setElement(IMTBChangerPtr element, int position) {
	if (!element) {
		return false;
	}
	return element->SetPosition(position, MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
}

int ZeissMTB_Erlangen::getElement(IMTBChangerPtr element) {
	if (!element) {
		return -1;
	}
	return element->GetPosition();
}

void ZeissMTB_Erlangen::setReflector(int position, bool block) {
	// Set the position
	auto success = setElement(m_Reflector, position);
}

int ZeissMTB_Erlangen::getReflector() {
	return getElement(m_Reflector);
}

void ZeissMTB_Erlangen::setObjective(int position, bool block) {
	// Set the position
	auto success = setElement(m_Objective, position);
}

void ZeissMTB_Erlangen::setSideport(int position, bool block) {
	// Set the position
	auto success = setElement(m_Sideport, position);
}

int ZeissMTB_Erlangen::getObjective() {
	return getElement(m_Objective);
}

int ZeissMTB_Erlangen::getSideport() {
	return getElement(m_Sideport);
}

void ZeissMTB_Erlangen::setRLShutter(int position, bool block) {
	// Set the position
	auto success = setElement(m_RLShutter, position);
}

int ZeissMTB_Erlangen::getRLShutter() {
	return getElement(m_RLShutter);
}

void ZeissMTB_Erlangen::setMirror(int position) {
	// calculate the position to set, slots are spaced every 32 mm
	double pos = 60.0 * ((double)position - 1);
	// This device has 1024 encoder pulses per mm
	pos *= 1024;
	m_Mirror->setPosition(pos);
}

int ZeissMTB_Erlangen::getMirror() {
	double pos = m_Mirror->getPosition();
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

void ZeissMTB_Erlangen::setBeamBlock(int position) {
	m_beamBlockOpen = position;
	// Write digital voltages
	const uInt8	voltage = (uInt8)m_beamBlockOpen;
	DAQmxWriteDigitalLines(DOtaskHandle_BeamBlock, 1, false, 10, DAQmx_Val_GroupByChannel, &voltage, NULL, NULL);
}

int ZeissMTB_Erlangen::getBeamBlock() {
	return (int)m_beamBlockOpen;
}