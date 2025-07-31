#include "stdafx.h"
#include <windows.h>

#include "NIDAQ.h"
#include "src/lib/math/simplemath.h"

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
		{ "Brillouin",		ScanPreset::SCAN_BRILLOUIN,		{ {2}, {1}, {1},  {},  {},  {}, {} }	},	// Brillouin
		{ "Calibration",	ScanPreset::SCAN_CALIBRATION,	{ {2}, {2}, {1},  {},  {},  {}, {} }	},	// Brillouin Calibration
		{ "ODT",			ScanPreset::SCAN_ODT,			{ {2},  {}, {2}, {1}, {1}, {1}, {} }	},	// ODT
		{ "Brightfield",	ScanPreset::SCAN_BRIGHTFIELD,	{  {},  {},  {}, {1}, {2}, {2}, {} }	},	// Brightfield
		{ "Fluo off",		ScanPreset::SCAN_EPIFLUOOFF,	{  {},  {},  {}, {1}, {1},  {}, {} }	},	// Fluorescence off
		{ "Fluo Blue",		ScanPreset::SCAN_EPIFLUOBLUE,	{ {1},  {},  {}, {2}, {2}, {1}, {} }	},	// Fluorescence blue
		{ "Fluo Green",		ScanPreset::SCAN_EPIFLUOGREEN,	{ {1},  {},  {}, {3}, {3}, {1}, {} }	},	// Fluorescence green
		{ "Fluo Red",		ScanPreset::SCAN_EPIFLUORED,	{ {1},  {},  {}, {4}, {4}, {1}, {} }	},	// Fluorescence red
		{ "Laser off",		ScanPreset::SCAN_LASEROFF,		{ {1},  {},  {},  {},  {},  {}, {} }	}	// Laser off
	};

	VoltageCalibrationHelper::calculateCalibrationWeights(&m_voltageCalibration);

	m_elementPositions = std::vector<double>((int)DEVICE_ELEMENT::COUNT, -1);

	// Register capabilities
	registerCapability(Capabilities::ODT);
	registerCapability(Capabilities::VoltageCalibration);
	registerCapability(Capabilities::LaserScanner);

	/*
	 * Initialize the scale calibration
	 */
	auto pixelSize = double{ 4.8 };	// [µm]  pixel size
	auto mag = double{ 57 };		// [1]   magnification
	auto scale = mag / pixelSize;

	auto scaleCalibration = ScaleCalibrationData{};
	scaleCalibration.micrometerToPixX = { scale, 0 };
	scaleCalibration.micrometerToPixY = { 0, scale };

	auto width = double{ 1280 };	// [pix] camera image width
	auto height = double{ 1024 };	// [pix] camera image height
	scaleCalibration.originPix = { width / 2, height / 2 };

	try {
		ScaleCalibrationHelper::initializeCalibrationFromMicrometer(&scaleCalibration);

		setScaleCalibration(scaleCalibration);
	} catch (std::exception& e) {
	}
}

NIDAQ::~NIDAQ() {
	disconnectDevice();
	if (m_elementPositionTimer) {
		m_elementPositionTimer->stop();
		m_elementPositionTimer->deleteLater();
	}
	if (m_exFilter) {
		m_exFilter->deleteLater();
		m_exFilter = nullptr;
	}
	if (m_emFilter) {
		m_emFilter->deleteLater();
		m_emFilter = nullptr;
	}
}

void NIDAQ::setPosition(POINT2 position) {
	// Since the NIDAQ class does not support capability TranslationStage,
	// m_positionStage is always { 0, 0 } and we don't have to handle it here.

	// Check if the position is in valid range.
	// This could also throw an exception in the future.
	// x-value
	if (position.x < m_absoluteBounds.xMin) {
		position.x = m_absoluteBounds.xMin;
	}
	if (position.x > m_absoluteBounds.xMax) {
		position.x = m_absoluteBounds.xMax;
	}
	// y-value
	if (position.y < m_absoluteBounds.yMin) {
		position.y = m_absoluteBounds.yMin;
	}
	if (position.y > m_absoluteBounds.yMax) {
		position.y = m_absoluteBounds.yMax;
	}

	m_positionScanner = position;

	// Set the scan position
	applyPosition();

	// Announce the updated positions
	announcePositions();
	announcePositionScanner();
}

void NIDAQ::setPosition(POINT3 position) {
	m_positionFocus = position.z;

	setPosition(POINT2{ position.x, position.y });
}

VOLTAGE2 NIDAQ::positionToVoltage(POINT2 position) {

	auto Uxr = interpolation::biharmonic_spline_calculate_values(m_voltageCalibration.positions_weights.x, position.x, position.y);
	auto Uyr = interpolation::biharmonic_spline_calculate_values(m_voltageCalibration.positions_weights.y, position.x, position.y);

	return VOLTAGE2{ Uxr, Uyr };
}

POINT2 NIDAQ::voltageToPosition(VOLTAGE2 voltage) {

	auto xr = interpolation::biharmonic_spline_calculate_values(m_voltageCalibration.voltages_weights.x, voltage.Ux, voltage.Uy);
	auto yr = interpolation::biharmonic_spline_calculate_values(m_voltageCalibration.voltages_weights.y, voltage.Ux, voltage.Uy);

	return POINT2{ xr, yr };
}

/*
 * Public slots
 */

void NIDAQ::init() {
	calculateHomePositionBounds();

	if (!m_exFilter) {
		m_exFilter = new FilterMount("COM3");
		m_exFilter->init();
	}
	if (!m_emFilter) {
		m_emFilter = new FilterMount("COM6");
		m_emFilter->init();
	}

	if (!m_elementPositionTimer) {
		m_elementPositionTimer = new QTimer();
		auto connection = QWidget::connect(
			m_elementPositionTimer,
			&QTimer::timeout,
			this,
			&NIDAQ::getElements
		);
	}
}

void NIDAQ::connectDevice() {
	if (!m_isConnected) {
		// Connect NIDAQ board
		ODTControl::connectDevice();

		// Connect to T-Cube Piezo Inertial Controller
		// Build list of connected device
		if (Thorlabs_TIM::TLI_BuildDeviceList() == 0) {
			auto ret = Thorlabs_TIM::TIM_Open(m_serialNo_TIM);
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

			ret = Thorlabs_FF::FF_Open(m_serialNo_FF1);
			if (ret == 0) {
				Thorlabs_FF::FF_StartPolling(m_serialNo_FF1, 200);
			}

			ret = Thorlabs_KSC::SC_Open(m_serialNo_KSC);
			if (ret == 0) {
				Thorlabs_KSC::SC_StartPolling(m_serialNo_KSC, 200);
				Thorlabs_KSC::SC_SetOperatingMode(m_serialNo_KSC, Thorlabs_KSC::SC_OperatingModes::SC_Manual);
			}

			ret = Thorlabs_KDC::CC_Open(m_serialNo_KDC);
			if (ret == 0) {
				Thorlabs_KDC::CC_StartPolling(m_serialNo_KDC, 200);
			}
		}

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

int NIDAQ::getElement(const DeviceElement& element) {
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
	m_elementPositionsTmp = m_elementPositions;
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::BEAMBLOCK] = getBeamBlock();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::CALFLIPMIRROR] = getCalFlipMirror();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::MOVEMIRROR] = getMirror();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::EXFILTER] = getExFilter();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::EMFILTER] = getEmFilter();
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::LEDLAMP] = (double)getLEDLamp() + 1;
	m_elementPositionsTmp[(int)DEVICE_ELEMENT::LOWEROBJECTIVE] = getLowerObjective();
	// We only emit changed positions
	if (m_elementPositionsTmp != m_elementPositions) {
		m_elementPositions = m_elementPositionsTmp;
		checkPresets();
		emit(elementPositionsChanged(m_elementPositions));
	}
}

void NIDAQ::setVoltageCalibration(VoltageCalibrationData voltageCalibration) {
	m_voltageCalibration = voltageCalibration;

	centerPosition();
	calculateHomePositionBounds();
}

void NIDAQ::setHome() {
	// Set current z position to zero
	m_positionFocus = 0;
	Thorlabs_TIM::TIM_Home(m_serialNo_TIM, m_channelPosZ);

	ScanControl::setHome();
}

/*
 * Private definitions
 */
void NIDAQ::setPresetAfter(ScanPreset presetType) {
	if (presetType == ScanPreset::SCAN_CALIBRATION) {
		// Set voltage of galvo mirrors to zero
		// Otherwise the laser beam might not hit the calibration samples
		setVoltage({ 0, 0 });
	}
}

void NIDAQ::calculateBounds() {
	// TODO: Would be good to untangle this from the camera parameters. Question is how.
	auto width = double{ 1280 };	// [pix] camera image width
	auto height = double{ 1024 };	// [pix] camera image height

	auto pos0 = pixToMicroMeter({ 0, 0 });
	auto pos1 = pixToMicroMeter({ 0, height });
	auto pos2 = pixToMicroMeter({ width, 0 });
	auto pos3 = pixToMicroMeter({ width, height });

	auto x = std::vector<double>{ pos0.x, pos1.x, pos2.x, pos3.x };
	auto y = std::vector<double>{ pos0.y, pos1.y, pos2.y, pos3.y };

	m_absoluteBounds.xMin = simplemath::minimum(x);
	m_absoluteBounds.xMax = simplemath::maximum(x);
	m_absoluteBounds.yMin = simplemath::minimum(y);
	m_absoluteBounds.yMax = simplemath::maximum(y);
}

void NIDAQ::applyPosition() {
	DAQmxStopTask(AOtaskHandle);
	// set the x- and y-position
	m_voltages = positionToVoltage(POINT2{ m_positionScanner.x, m_positionScanner.y });
	float64 data[2] = { m_voltages.Ux, m_voltages.Uy };
	DAQmxWriteAnalogF64(AOtaskHandle, 1, true, 10.0, DAQmx_Val_GroupByChannel, data, NULL, NULL);
	// set the z-position
	Thorlabs_TIM::TIM_MoveAbsolute(m_serialNo_TIM, m_channelPosZ, m_PiezoIncPerMum * m_positionFocus);
	calculateCurrentPositionBounds();
	announcePosition();
}

void NIDAQ::centerPosition() {
	// Set current focus position to zero
	Thorlabs_TIM::TIM_Home(m_serialNo_TIM, m_channelPosZ);

	setPosition({ 0, 0, 0 });
}

void NIDAQ::setFilter(FilterMount* device, int position) {
	// calculate the position to set, slots are spaced every 32 mm
	auto pos = 32.0 * ((double)position - 1);
	device->setPosition(pos);
}

int NIDAQ::getFilter(FilterMount* device) {
	auto pos = device->getPosition();
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
	auto realPosition{ 0.0 };
	if (position == 1) {
		realPosition = m_mirrorStart;
	} else if (position == 2) {
		realPosition = m_mirrorEnd;
	}
	auto incPos = realPosition * m_gearBoxRatio * m_stepsPerRev / m_pitch;
	Thorlabs_KDC::CC_MoveToPosition(m_serialNo_KDC, incPos);

	// check if motor is still moving
	auto messageType = WORD{};
	auto messageId = WORD{};
	auto messageData = DWORD{};
	Thorlabs_KDC::CC_WaitForMessage(m_serialNo_KDC, &messageType, &messageId, &messageData);
	while (messageType != 2 || messageId != 1) {
		Thorlabs_KDC::CC_WaitForMessage(m_serialNo_KDC, &messageType, &messageId, &messageData);
	}

	auto currentPos = Thorlabs_KDC::CC_GetPosition(m_serialNo_KDC);
}

int NIDAQ::getMirror() {
	auto currentIndex = Thorlabs_KDC::CC_GetPosition(m_serialNo_KDC);
	// position 1
	auto targetIndex = (int)(m_mirrorStart * m_gearBoxRatio * m_stepsPerRev / m_pitch);
	if (abs(currentIndex - targetIndex) < 10) {
		return 1;
	}
	// position 2
	targetIndex = (int)(m_mirrorEnd * m_gearBoxRatio * m_stepsPerRev / m_pitch);
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