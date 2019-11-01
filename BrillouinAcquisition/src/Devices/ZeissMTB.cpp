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
		{ "Mirror",		2, (int)DEVICE_ELEMENT::MIRROR },
		{ "Hal. Lamp",	0, (int)DEVICE_ELEMENT::LAMP, DEVICE_INPUT_TYPE::SLIDER }
	};

	m_presets = {
		{	"Brillouin",	ScanPreset::SCAN_BRILLOUIN,		{ {2}, {}, {1}, {3}, {1}, {2},  {}, {} } },	// Brillouin
		{	"Calibration",	ScanPreset::SCAN_CALIBRATION,	{ {2}, {}, {1}, {3}, {1}, {3},  {}, {} } },	// Calibration
		{	"Brightfield",	ScanPreset::SCAN_BRIGHTFIELD,	{ {2}, {}, {1}, {3}, {1}, {2}, {2}, {} } },	// Brightfield
		{	"Eyepiece",		ScanPreset::SCAN_EYEPIECE,		{ {2}, {}, {1}, {3}, {2}, {3}, {2}, {} } },	// Eyepiece
		{	"Fluo Blue",	ScanPreset::SCAN_EPIFLUOBLUE,	{ {1}, {}, {2}, {3}, {},  {2}, {1}, {} } },	// Fluorescence blue
		{	"Fluo Green",	ScanPreset::SCAN_EPIFLUOGREEN,	{ {1}, {}, {3}, {3}, {},  {2}, {1}, {} } },	// Fluorescence green
		{	"Fluo Red",		ScanPreset::SCAN_EPIFLUORED,	{ {1}, {}, {4}, {3}, {},  {2}, {1}, {} } },	// Fluorescence red
		{	"Laser off",	ScanPreset::SCAN_LASEROFF,		{ {1}, {},  {},  {}, {},   {},  {}, {} } }	// Laser off
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

	delete m_focus;
	delete m_mcu;
	delete m_stand;
	delete m_comObject;
}

void ZeissMTB::init() {
	m_comObject = new com();

	m_focus = new Focus(m_comObject);
	m_mcu = new MCU(m_comObject);
	m_stand = new Stand(m_comObject);

	/*
	 * Initialize Zeiss MTB handles
	 */
	CoInitialize(NULL);
	try {
		// create an instance of the connection class which can connect to the server
		m_MTBConnection = IMTBConnectionPtr(CLSID_MTBConnection);
	} catch (_com_error e) {
	}

	QMetaObject::Connection connection = QWidget::connect(
		m_comObject,
		SIGNAL(errorOccurred(QSerialPort::SerialPortError)),
		this,
		SLOT(errorHandler(QSerialPort::SerialPortError))
	);

	positionTimer = new QTimer();
	connection = QWidget::connect(positionTimer, SIGNAL(timeout()), this, SLOT(announcePosition()));

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
			m_isMTBConnected = true;
			/*
			 * Get the device handles
			 */
			m_Stand = (IUnknown*)(m_Root->GetDevice(0));	// Stand handle
			if (m_Stand) {
				// Try to get the Halogen Lamp handle
				m_Lamp = (IMTBContinualPtr)m_Stand->GetComponent("MTBTLHalogenLamp");
				//_bstr_t name = ((IMTBIdentPtr)m_Lamp)->GetName();
			}

			m_comObject->setPortName("COM1");
			if (!m_comObject->setBaudRate(QSerialPort::Baud9600)) {
				throw QString("Could not set BaudRate.");
			}
			if (!m_comObject->setFlowControl(QSerialPort::HardwareControl)) {
				throw QString("Could not set FlowControl.");
			}
			if (!m_comObject->setDataBits(QSerialPort::Data8)) {
				throw QString("Could not set DataBits.");
			}
			if (!m_comObject->setParity(QSerialPort::NoParity)) {
				throw QString("Could not set Parity.");
			}
			if (!m_comObject->setStopBits(QSerialPort::OneStop)) {
				throw QString("Could not set StopBits.");
			}
			m_isConnected = m_comObject->open(QIODevice::ReadWrite);
			if (!m_isConnected) {
				throw QString("Could not open the serial port.");
			}
			m_comObject->clear();

			int baudRate = m_comObject->baudRate();
			QSerialPort::DataBits dataBits = m_comObject->dataBits();
			QSerialPort::FlowControl flowControl = m_comObject->flowControl();
			QSerialPort::Parity parity = m_comObject->parity();
			QSerialPort::StopBits stopBits = m_comObject->stopBits();

			Thorlabs_FF::FF_Open(m_serialNo_FF2);
			Thorlabs_FF::FF_StartPolling(m_serialNo_FF2, 200);

			// check if connected to compatible device
			bool focus = m_focus->checkCompatibility();
			bool stand = m_stand->checkCompatibility();
			bool mcu = m_mcu->checkCompatibility();

			m_isCompatible = focus && stand && mcu;

			if (m_isConnected && m_isCompatible && m_isMTBConnected) {
				setPreset(ScanPreset::SCAN_BRILLOUIN);
				getElements();
				m_homePosition = getPosition();
				startAnnouncingPosition();
				startAnnouncingElementPosition();
				calculateHomePositionBounds();
				calculateCurrentPositionBounds();
			} else {
				m_isConnected = false;
			}

		} catch (QString e) {
			// todo
		}
	}
	emit(connectedDevice(m_isConnected && m_isCompatible));
}

void ZeissMTB::disconnectDevice() {
	if (m_comObject && m_isConnected) {
		stopAnnouncingPosition();
		stopAnnouncingElementPosition();
		m_comObject->close();
		Thorlabs_FF::FF_Close(m_serialNo_FF2);
		Thorlabs_FF::FF_StopPolling(m_serialNo_FF2);
		m_isConnected = false;
		m_isCompatible = false;
	}
	if (m_isMTBConnected) {
		if (m_MTBConnection != NULL && m_ID != "") {
			// logout from MTB
			try {
				m_MTBConnection->Logout((BSTR)m_ID);
			} catch (_com_error e) {
			}

			m_MTBConnection->Close();
			m_isMTBConnected = false;
		}
	}
	emit(connectedDevice(m_isConnected && m_isCompatible));
}

void ZeissMTB::errorHandler(QSerialPort::SerialPortError error) {
}

void ZeissMTB::setPosition(POINT3 position) {
	m_mcu->setX(position.x);
	m_mcu->setY(position.y);
	m_focus->setZ(position.z);
	calculateCurrentPositionBounds(position);
}

void ZeissMTB::setPosition(POINT2 position) {
	m_mcu->setX(position.x);
	m_mcu->setY(position.y);
	calculateCurrentPositionBounds();
}

void ZeissMTB::setPositionRelativeX(double positionX) {
	m_mcu->setX(positionX + m_homePosition.x);
	calculateCurrentPositionBounds();
}

void ZeissMTB::setPositionRelativeY(double positionY) {
	m_mcu->setY(positionY + m_homePosition.y);
	calculateCurrentPositionBounds();
}

void ZeissMTB::setPositionRelativeZ(double positionZ) {
	m_focus->setZ(positionZ + m_homePosition.z);
	calculateCurrentPositionBounds();
}

void ZeissMTB::setPositionInPix(POINT2) {
	// Does nothing for now, since for the 780 nm setup no spatial calibration is in place yet.
}

POINT3 ZeissMTB::getPosition() {
	double x = m_mcu->getX();
	double y = m_mcu->getY();
	double z = m_focus->getZ();
	return POINT3{ x, y, z };
}

void ZeissMTB::setDevice(com *device) {
	delete m_comObject;
	m_comObject = device;
	m_focus->setDevice(device);
	m_mcu->setDevice(device);
	m_stand->setDevice(device);
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

void ZeissMTB::setElement(DeviceElement element, double position) {
	switch ((DEVICE_ELEMENT)element.index) {
		case DEVICE_ELEMENT::BEAMBLOCK:
			setBeamBlock((int)position);
			break;
		case DEVICE_ELEMENT::REFLECTOR:
			m_stand->setReflector((int)position, true);
			break;
		case DEVICE_ELEMENT::OBJECTIVE:
			m_stand->setObjective((int)position, true);
			break;
		case DEVICE_ELEMENT::TUBELENS:
			m_stand->setTubelens((int)position, true);
			break;
		case DEVICE_ELEMENT::BASEPORT:
			m_stand->setBaseport((int)position, true);
			break;
		case DEVICE_ELEMENT::SIDEPORT:
			m_stand->setSideport((int)position, true);
			break;
		case DEVICE_ELEMENT::MIRROR:
			m_stand->setMirror((int)position, true);
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

void ZeissMTB::getElements() {
	m_elementPositionsTmp = m_elementPositions;
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::BEAMBLOCK] = getBeamBlock();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::REFLECTOR] = m_stand->getReflector();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::OBJECTIVE] = m_stand->getObjective();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::TUBELENS] = m_stand->getTubelens();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::BASEPORT] = m_stand->getBaseport();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::SIDEPORT] = m_stand->getSideport();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::MIRROR] = m_stand->getMirror();
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

void ZeissMTB::getElement(DeviceElement element) {
}

double ZeissMTB::getLamp() {
	return m_Lamp->GetPosition("%");
}

void ZeissMTB::setLamp(int voltage, bool block) {
	if (voltage > 100) voltage = 100;
	if (voltage < 0) voltage = 0;
	// Set the voltage
	auto success = m_Lamp->SetPosition(voltage, "%", MTBCmdSetModes::MTBCmdSetModes_Synchronous, 500);
}