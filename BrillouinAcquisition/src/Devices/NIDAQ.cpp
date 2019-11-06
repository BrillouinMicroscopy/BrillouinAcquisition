#include "stdafx.h"
#include "NIDAQ.h"
#include <windows.h>

/*
 * Public definitions
 */

NIDAQ::NIDAQ() noexcept {

	m_deviceElements = {
		{ "Beam Block",			2, (int)DEVICE_ELEMENT::BEAMBLOCK,		{ "Close", "Open" } },
		{ "Flip Mirror",		2, (int)DEVICE_ELEMENT::CALFLIPMIRROR,	{ "Open", "Reflect" } },
		{ "Moveable Mirror",	2, (int)DEVICE_ELEMENT::MOVEMIRROR,		{ "Reflect", "Open" } },
		{ "Excitation Filter",	4, (int)DEVICE_ELEMENT::EXFILTER,		{ "Block", "Blue", "Green", "Red" } },
		{ "Emission Filter",	4, (int)DEVICE_ELEMENT::EMFILTER,		{ "Open", "Blue", "Green", "Red" } },
		{ "LED illumination",	2, (int)DEVICE_ELEMENT::LEDLAMP,		{ "Off", "On" } },
		{ "Lower objective",	0, (int)DEVICE_ELEMENT::LOWEROBJECTIVE,	DEVICE_INPUT_TYPE::DOUBLEBOX }
	};

	m_presets = {
		{	"Brillouin",	ScanPreset::SCAN_BRILLOUIN,		{ {2}, {1}, {1},  {},  {},  {},  {} }	},	// Brillouin
		{	"Calibration",	ScanPreset::SCAN_CALIBRATION,	{ {2}, {2}, {1},  {},  {},  {},  {} }	},	// Brillouin Calibration
		{	"ODT",			ScanPreset::SCAN_ODT,			{ {2},  {}, {2}, {1}, {1}, {1},  {} }	},	// ODT
		{	"Brightfield",	ScanPreset::SCAN_BRIGHTFIELD,	{  {},  {},  {},  {}, {2}, {2},  {} }	},	// Brightfield
		{	"Fluo off",		ScanPreset::SCAN_EPIFLUOOFF,	{  {},  {},  {}, {1}, {1},  {},  {} }	},	// Fluorescence off
		{	"Fluo Blue",	ScanPreset::SCAN_EPIFLUOBLUE,	{ {1},  {},  {}, {2}, {2}, {1},  {} }	},	// Fluorescence blue
		{	"Fluo Green",	ScanPreset::SCAN_EPIFLUOGREEN,	{ {1},  {},  {}, {3}, {3}, {1},  {} }	},	// Fluorescence green
		{	"Fluo Red",		ScanPreset::SCAN_EPIFLUORED,	{ {1},  {},  {}, {4}, {4}, {1},  {} }	},	// Fluorescence red
		{	"Laser off",	ScanPreset::SCAN_LASEROFF,		{ {1},  {},  {},  {},  {},  {},  {} }	}	// Laser off
	};

	CalibrationHelper::calculateCalibrationBounds(&m_calibration);
	CalibrationHelper::calculateCalibrationWeights(&m_calibration);

	m_absoluteBounds = m_calibration.bounds;

	m_elementPositions = std::vector<double>((int)DEVICE_ELEMENT::COUNT, -1);
}

NIDAQ::~NIDAQ() {
	elementPositionTimer->stop();
	disconnectDevice();
}

void NIDAQ::setPosition(POINT2 newPosition) {
	POINT3 position = m_position;
	position.x = newPosition.x;
	position.y = newPosition.y;
	setPosition(position);
}

void NIDAQ::setPosition(POINT3 position) {
	// check if position is in valid range
	// this could also throw an exception in the future
	// x-value
	if (position.x < m_calibration.bounds.xMin) {
		position.x = m_calibration.bounds.xMin;
	}
	if (position.x > m_calibration.bounds.xMax) {
		position.x = m_calibration.bounds.xMax;
	}
	// y-value
	if (position.y < m_calibration.bounds.yMin) {
		position.y = m_calibration.bounds.yMin;
	}
	if (position.y > m_calibration.bounds.yMax) {
		position.y = m_calibration.bounds.yMax;
	}

	m_position = position;
	// set the scan position
	applyScanPosition();
}

POINT3 NIDAQ::getPosition() {
	return m_position;
}

/*
 * Public slots
 */

void NIDAQ::init() {
	calculateHomePositionBounds();

	m_exFilter = new FilterMount("COM3");
	m_exFilter->init();
	m_emFilter = new FilterMount("COM6");
	m_emFilter->init();

	elementPositionTimer = new QTimer();
	QMetaObject::Connection connection = QWidget::connect(elementPositionTimer, SIGNAL(timeout()), this, SLOT(getElements()));
}

void NIDAQ::connectDevice() {
	if (!m_isConnected) {
		// Connect NIDAQ board
		ODTControl::connectDevice();

		// Connect to T-Cube Piezo Inertial Controller
		int ret = Thorlabs_TIM::TIM_Open(m_serialNo_TIM);
		if (ret == 0) {
			Thorlabs_TIM::TIM_Enable(m_serialNo_TIM);
			Thorlabs_TIM::TIM_StartPolling(m_serialNo_TIM, 200);
			m_isConnected = true;
			m_isCompatible = true;
			centerPosition();
			calculateHomePositionBounds();
			Thorlabs_TIM::TIM_Home(m_serialNo_TIM, m_channelLowerObjective);
			m_positionLowerObjective = 0;
		}
		Thorlabs_FF::FF_Open(m_serialNo_FF1);
		Thorlabs_FF::FF_StartPolling(m_serialNo_FF1, 200);

		Thorlabs_KSC::SC_Open(m_serialNo_KSC);
		Thorlabs_KSC::SC_StartPolling(m_serialNo_KSC, 200);
		Thorlabs_KSC::SC_SetOperatingMode(m_serialNo_KSC, Thorlabs_KSC::SC_OperatingModes::SC_Manual);

		Thorlabs_KDC::CC_Open(m_serialNo_KDC);
		Thorlabs_KDC::CC_StartPolling(m_serialNo_KDC, 200);

		// Connect filter mounts
		m_exFilter->connectDevice();
		m_emFilter->connectDevice();

		startAnnouncingElementPosition();
		getElements();
	}
	emit(connectedDevice(m_isConnected && m_isCompatible));
}

void NIDAQ::disconnectDevice() {
	if (m_isConnected) {
		stopAnnouncingElementPosition();
		ODTControl::disconnectDevice();

		// Disconnect from T-Cube Piezo Inertial Controller
		Thorlabs_TIM::TIM_StopPolling(m_serialNo_TIM);
		Thorlabs_TIM::TIM_Disconnect(m_serialNo_TIM);
		Thorlabs_TIM::TIM_Close(m_serialNo_TIM);

		Thorlabs_FF::FF_Close(m_serialNo_FF1);
		Thorlabs_FF::FF_StopPolling(m_serialNo_FF1);
		Thorlabs_KSC::SC_Close(m_serialNo_KSC);
		Thorlabs_KSC::SC_StopPolling(m_serialNo_KSC);

		Thorlabs_KDC::CC_StopPolling(m_serialNo_KDC);
		Thorlabs_KDC::CC_Close(m_serialNo_KDC);

		// Disconnect filter mounts
		m_exFilter->disconnectDevice();
		m_emFilter->disconnectDevice();

		m_isConnected = false;
		m_isCompatible = false;
	}
	emit(connectedDevice(m_isConnected && m_isCompatible));
}

void NIDAQ::setElement(DeviceElement element, double position) {
	switch ((DEVICE_ELEMENT)element.index) {
		case DEVICE_ELEMENT::CALFLIPMIRROR:
			setCalFlipMirror((int)position);
			break;
		case DEVICE_ELEMENT::BEAMBLOCK:
			setBeamBlock((int)position);
			break;
		case DEVICE_ELEMENT::MOVEMIRROR:
			setMirror((int)position);
			break;
		case DEVICE_ELEMENT::EXFILTER:
			setExFilter((int)position);
			break;
		case DEVICE_ELEMENT::EMFILTER:
			setEmFilter((int)position);
			break;
		case DEVICE_ELEMENT::LEDLAMP:
			setLEDLamp((int)position - 1);
			break;
		case DEVICE_ELEMENT::LOWEROBJECTIVE:
			setLowerObjective(position);
			break;
		default:
			break;
	}
	m_elementPositions[element.index] = position;
	checkPresets();
	emit(elementPositionChanged(element, position));
}

int NIDAQ::getElement(DeviceElement element) {
	switch ((DEVICE_ELEMENT)element.index) {
		case DEVICE_ELEMENT::BEAMBLOCK:
			m_elementPositions[element.index] = getBeamBlock();
			break;
		case DEVICE_ELEMENT::CALFLIPMIRROR:
			m_elementPositions[element.index] = getCalFlipMirror();
			break;
		case DEVICE_ELEMENT::MOVEMIRROR:
			m_elementPositions[element.index] = getMirror();
			break;
		case DEVICE_ELEMENT::EXFILTER:
			m_elementPositions[element.index] = getExFilter();
			break;
		case DEVICE_ELEMENT::EMFILTER:
			m_elementPositions[element.index] = getEmFilter();
			break;
		case DEVICE_ELEMENT::LEDLAMP:
			m_elementPositions[element.index] = (double)getLEDLamp() + 1;
			break;
		case DEVICE_ELEMENT::LOWEROBJECTIVE:
			m_elementPositions[element.index] = getLowerObjective();
			break;
		default:
			return -1;
	}
	checkPresets();
	emit(elementPositionChanged(element, m_elementPositions[element.index]));
	return m_elementPositions[element.index];
}

void NIDAQ::getElements() {
	m_elementPositions[(int)DEVICE_ELEMENT::BEAMBLOCK] = getBeamBlock();
	m_elementPositions[(int)DEVICE_ELEMENT::CALFLIPMIRROR] = getCalFlipMirror();
	m_elementPositions[(int)DEVICE_ELEMENT::MOVEMIRROR] = getMirror();
	m_elementPositions[(int)DEVICE_ELEMENT::EXFILTER] = getExFilter();
	m_elementPositions[(int)DEVICE_ELEMENT::EMFILTER] = getEmFilter();
	m_elementPositions[(int)DEVICE_ELEMENT::LEDLAMP] = (double)getLEDLamp() + 1;
	m_elementPositions[(int)DEVICE_ELEMENT::LOWEROBJECTIVE] = getLowerObjective();
	checkPresets();
	emit(elementPositionsChanged(m_elementPositions));
}

void NIDAQ::setPreset(ScanPreset presetType) {
	auto preset = getPreset(presetType);

	for (gsl::index ii = 0; ii < m_deviceElements.size(); ii++) {
		// check if element position needs to be changed
		if (!preset.elementPositions[ii].empty() && !simplemath::contains(preset.elementPositions[ii], m_elementPositions[ii])) {
			setElement(m_deviceElements[ii], preset.elementPositions[ii][0]);
			m_elementPositions[ii] = preset.elementPositions[ii][0];
		}
	}

	if (presetType == ScanPreset::SCAN_CALIBRATION) {
		// Set voltage of galvo mirrors to zero
		// Otherwise the laser beam might not hit the calibration samples
		setVoltage({ 0, 0 });
	}

	checkPresets();
	emit(elementPositionsChanged(m_elementPositions));
}

void NIDAQ::setPositionRelativeX(double positionX) {
	m_position.x = positionX + m_homePosition.x;
	setPosition(m_position);
}

void NIDAQ::setPositionRelativeY(double positionY) {
	m_position.y = positionY + m_homePosition.y;
	setPosition(m_position);
}

void NIDAQ::setPositionRelativeZ(double positionZ) {
	m_position.z = positionZ + m_homePosition.z;
	setPosition(m_position);
}

void NIDAQ::setPositionInPix(POINT2 positionPix) {
	POINT2 positionMicroMeter = pixToMicroMeter(positionPix);

	setPosition(positionMicroMeter);
}

void NIDAQ::setSpatialCalibration(SpatialCalibration spatialCalibration) {
	m_calibration = spatialCalibration;

	centerPosition();
	calculateHomePositionBounds();

	m_absoluteBounds = m_calibration.bounds;
}

void NIDAQ::setHome() {
	// Set current z position to zero
	m_position.z = 0;
	Thorlabs_TIM::TIM_Home(m_serialNo_TIM, m_channelPosZ);

	ScanControl::setHome();
}

/*
 * Private definitions
 */

/*
 * Function converts a position in pixel to a position im µm
 * by taking into account the pixel size and magnification.
 * The center of the camera image is the point of origin (0,0).
 */
POINT2 NIDAQ::pixToMicroMeter(POINT2 positionPix) {

	POINT2 positionMicroMeter;
	positionMicroMeter.x = 1e6 * (positionPix.x - m_calibration.microscopeProperties.width / 2) * m_calibration.microscopeProperties.pixelSize / m_calibration.microscopeProperties.mag;
	positionMicroMeter.y = 1e6 * (positionPix.y - m_calibration.microscopeProperties.height / 2) * m_calibration.microscopeProperties.pixelSize / m_calibration.microscopeProperties.mag;

	return positionMicroMeter;
}

void NIDAQ::applyScanPosition() {
	DAQmxStopTask(AOtaskHandle);
	// set the x- and y-position
	m_voltages = positionToVoltage(POINT2{ 1e-6 * m_position.x, 1e-6 * m_position.y });
	float64 data[2] = { m_voltages.Ux, m_voltages.Uy };
	DAQmxWriteAnalogF64(AOtaskHandle, 1, true, 10.0, DAQmx_Val_GroupByChannel, data, NULL, NULL);
	// set the z-position
	Thorlabs_TIM::TIM_MoveAbsolute(m_serialNo_TIM, m_channelPosZ, m_PiezoIncPerMum * m_position.z);
	calculateCurrentPositionBounds();
	announcePosition();
}

void NIDAQ::centerPosition() {
	m_position = { 0, 0, 0 };
	// Set current position to zero
	Thorlabs_TIM::TIM_Home(m_serialNo_TIM, m_channelPosZ);
	// set the scan position
	applyScanPosition();
}

VOLTAGE2 NIDAQ::positionToVoltage(POINT2 position) {

	auto Uxr = interpolation::biharmonic_spline_calculate_values(m_calibration.positions_weights.x, position.x, position.y);
	auto Uyr = interpolation::biharmonic_spline_calculate_values(m_calibration.positions_weights.y, position.x, position.y);

	return VOLTAGE2{ Uxr, Uyr };
}

POINT2 NIDAQ::voltageToPosition(VOLTAGE2 voltage) {

	auto xr = interpolation::biharmonic_spline_calculate_values(m_calibration.voltages_weights.x, voltage.Ux, voltage.Uy);
	auto yr = interpolation::biharmonic_spline_calculate_values(m_calibration.voltages_weights.y, voltage.Ux, voltage.Uy);

	return POINT2{ xr, yr };
}

void NIDAQ::setFilter(FilterMount* device, int position) {
	// calculate the position to set, slots are spaced every 32 mm
	double pos = 32.0 * ((double)position - 1);
	device->setPosition(pos);
}

int NIDAQ::getFilter(FilterMount* device) {
	double pos = device->getPosition();
	// Somehow the filter mount does not position the filters very accurately.
	// It can be off by multiple millimeters and the error increases with positions farther away.
	// E.g. requested 0 -> got 0, 32 -> 31, 64 -> 62, 96 -> 93
	for (gsl::index position{ 0 }; position < 4; position++) {
		if (abs(pos - 32.0 * position) < (1.0 + position)) {
			return (position + 1);
		}
	}
	return -1;
}

void NIDAQ::setCalFlipMirror(int position) {
	Thorlabs_FF::FF_MoveToPosition(m_serialNo_FF1, (Thorlabs_FF::FF_Positions)position);
}

int NIDAQ::getCalFlipMirror() {
	return Thorlabs_FF::FF_GetPosition(m_serialNo_FF1);
}

void NIDAQ::setBeamBlock(int position) {
	if (position == 1) {
		position = 2;
	} else {
		position = 1;
	}
	Thorlabs_KSC::SC_SetOperatingState(m_serialNo_KSC, (Thorlabs_KSC::SC_OperatingStates)position);
}

int NIDAQ::getBeamBlock() {
	int position = Thorlabs_KSC::SC_GetSolenoidState(m_serialNo_KSC);
	if (position == 1) {
		position = 2;
	} else {
		position = 1;
	}
	return position;
}

void NIDAQ::setMirror(int position) {
	double realPosition{ 0 };
	if (position == 1) {
		realPosition = 2.0;
	} else if (position == 2) {
		realPosition = 18.0;
	}
	int incPos = realPosition * m_gearBoxRatio * m_stepsPerRev / m_pitch;
	Thorlabs_KDC::CC_MoveToPosition(m_serialNo_KDC, incPos);

	// check if motor is still moving
	WORD messageType;
	WORD messageId;
	DWORD messageData;
	Thorlabs_KDC::CC_WaitForMessage(m_serialNo_KDC, &messageType, &messageId, &messageData);
	while (messageType != 2 || messageId != 1) {
		Thorlabs_KDC::CC_WaitForMessage(m_serialNo_KDC, &messageType, &messageId, &messageData);
	}

	int currentPos = Thorlabs_KDC::CC_GetPosition(m_serialNo_KDC);
}

int NIDAQ::getMirror() {
	int currentIndex = Thorlabs_KDC::CC_GetPosition(m_serialNo_KDC);
	// position 1
	double realPosition = 2.0;
	int targetIndex = realPosition * m_gearBoxRatio * m_stepsPerRev / m_pitch;
	if (abs(currentIndex - targetIndex) < 10) {
		return 1;
	}
	// position 2
	realPosition = 18.0;
	targetIndex = realPosition * m_gearBoxRatio * m_stepsPerRev / m_pitch;
	if (abs(currentIndex - targetIndex) < 10) {
		return 2;
	}
	return -1;
}

void NIDAQ::setEmFilter(int position) {
	setFilter(m_emFilter, position);
}

int NIDAQ::getEmFilter() {
	return getFilter(m_emFilter);
}

void NIDAQ::setExFilter(int position) {
	setFilter(m_exFilter, position);
}

int NIDAQ::getExFilter() {
	return getFilter(m_exFilter);
}

void NIDAQ::setLowerObjective(double position) {
	m_positionLowerObjective = position;
	Thorlabs_TIM::TIM_MoveAbsolute(m_serialNo_TIM, m_channelLowerObjective, m_PiezoIncPerMum * m_positionLowerObjective);
}

double NIDAQ::getLowerObjective() {
	return m_positionLowerObjective;
}