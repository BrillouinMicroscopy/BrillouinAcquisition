#include "stdafx.h"
#include "NIDAQ.h"

VOLTAGE2 NIDAQ::positionToVoltage(POINT2 position) {

	position = position - m_calibration.translation;

	double x_rot = position.x * cos(m_calibration.rho) + position.y * sin(m_calibration.rho);
	double y_rot = position.y * cos(m_calibration.rho) - position.x * sin(m_calibration.rho);

	double R_old = sqrt(pow(position.x, 2) + pow(position.y, 2));

	VOLTAGE2 voltage{ 0, 0 };
	if (R_old == 0) {
		voltage = { x_rot, y_rot };
		return voltage;
	}

	// solve for R_new
	
	COEFFICIANTS5 coef = m_calibration.coef;
	coef.e = -1 * R_old;
	std::vector<std::complex<double>> solutions = simplemath::solveQuartic(coef);

	// select real, positive solutions
	std::vector<double> selected;
	for (gsl::index i = 0; i < solutions.size(); i++) {
		if (abs((solutions[i]).imag()) < 1e-16 && solutions[i].real() > 0) {
			selected.push_back(solutions[i].real());
		}
	};

	double R_new{ 0 };
	if (selected.empty()) {
		R_new = NAN;
	} else if (selected.size() == 1) {
		R_new = selected[0];
	} else {
		R_new = simplemath::min(selected);
	}

	voltage = { x_rot / R_old * R_new, y_rot / R_old * R_new };
	return voltage;
}

POINT2 NIDAQ::voltageToPosition(VOLTAGE2 voltage) {

	double R_old = sqrt(pow(voltage.Ux, 2) + pow(voltage.Uy, 2));
	double R_new = m_calibration.coef.d * R_old + m_calibration.coef.c * pow(R_old, 2) +
				   m_calibration.coef.b * pow(R_old, 3) + m_calibration.coef.a * pow(R_old, 4);

	double Ux_rot = voltage.Ux * cos(m_calibration.rho) - voltage.Uy * sin(m_calibration.rho);
	double Uy_rot = voltage.Ux * sin(m_calibration.rho) + voltage.Uy * cos(m_calibration.rho);

	POINT2 position{ 0, 0 };
	if (R_old == 0) {
		position = { Ux_rot, Uy_rot };
	} else {
		position = { Ux_rot / R_old * R_new, Uy_rot / R_old * R_new };
	}
	
	position = position + m_calibration.translation;

	return position;
}

NIDAQ::NIDAQ() noexcept {
	m_presets = {
		{ 1, 2, 1 },	// Brillouin
		{ 2, 2, 1 },	// Calibration
		{ 2, 2, 2 },	// ODT
	};
	m_availablePresets = { 2, 1, 4 };

	m_absoluteBounds = m_calibration.bounds;

	m_deviceElements = {
		{ "Flip Mirror",	2, (int)DEVICE_ELEMENT::CALFLIPMIRROR },
		{ "Beam Block",		2, (int)DEVICE_ELEMENT::BEAMBLOCK },
		{ "Moveable Mirror",2, (int)DEVICE_ELEMENT::MOVEMIRROR }
	};
}

NIDAQ::~NIDAQ() {
	elementPositionTimer->stop();
	disconnectDevice();
}

void NIDAQ::connectDevice() {
	if (!m_isConnected) {
		// Create task for analog output
		DAQmxCreateTask("AO", &AOtaskHandle);
		// Configure analog output channels
		DAQmxCreateAOVoltageChan(AOtaskHandle, "Dev1/ao0:1", "AO", -1.0, 1.0, DAQmx_Val_Volts, "");
		// Configure sample rate to 1000 Hz
		DAQmxCfgSampClkTiming(AOtaskHandle, "", 1000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 1000);

		// Set analog output to zero
		float64 data[2] = { 0, 0 };
		DAQmxWriteAnalogF64(AOtaskHandle, 1, false, 10.0, DAQmx_Val_GroupByChannel, data, NULL, NULL);

		DAQmxSetWriteAttribute(AOtaskHandle, DAQmx_Write_RegenMode, DAQmx_Val_DoNotAllowRegen);

		// Create task for digital output
		DAQmxCreateTask("DO", &DOtaskHandle);
		// Configure digital output channel
		DAQmxCreateDOChan(DOtaskHandle, "Dev1/Port0/Line0:0", "DO", DAQmx_Val_ChanForAllLines);
		// Configure sample rate to 1000 Hz
		DAQmxCfgSampClkTiming(DOtaskHandle, "/Dev1/ao/SampleClock", 1000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 1000);

		// Set digital line to low
		DAQmxWriteDigitalLines(DOtaskHandle, 1, false, 10, DAQmx_Val_GroupByChannel, &m_TTL.low, NULL, NULL);

		DAQmxSetWriteAttribute(DOtaskHandle, DAQmx_Write_RegenMode, DAQmx_Val_DoNotAllowRegen);

		// Start digital task
		DAQmxStartTask(DOtaskHandle);

		// Start analog task after digital task since AO is the master
		DAQmxStartTask(AOtaskHandle);


		// Connect to T-Cube Piezo Inertial Controller
		int ret = Thorlabs_TIM::TIM_Open(m_serialNo_TIM);
		if (ret == 0) {
			Thorlabs_TIM::TIM_Enable(m_serialNo_TIM);
			Thorlabs_TIM::TIM_StartPolling(m_serialNo_TIM, 200);
			m_isConnected = true;
			m_isCompatible = true;
			centerPosition();
			calculateHomePositionBounds();
		}
		Thorlabs_FF::FF_Open(m_serialNo_FF1);
		Thorlabs_FF::FF_StartPolling(m_serialNo_FF1, 200);
		Thorlabs_FF::FF_Open(m_serialNo_FF2);
		Thorlabs_FF::FF_StartPolling(m_serialNo_FF2, 200);

		Thorlabs_KDC::CC_Open(m_serialNo_KDC);
		Thorlabs_KDC::CC_StartPolling(m_serialNo_KDC, 200);

		startAnnouncingElementPosition();
		getElements();
	}
	emit(connectedDevice(m_isConnected && m_isCompatible));
}

void NIDAQ::disconnectDevice() {
	if (m_isConnected) {
		stopAnnouncingElementPosition();
		// Stop and clear DAQ tasks
		DAQmxStopTask(AOtaskHandle);
		DAQmxClearTask(AOtaskHandle);
		DAQmxStopTask(DOtaskHandle);
		DAQmxClearTask(DOtaskHandle);

		// Disconnect from T-Cube Piezo Inertial Controller
		Thorlabs_TIM::TIM_StopPolling(m_serialNo_TIM);
		Thorlabs_TIM::TIM_Disconnect(m_serialNo_TIM);
		Thorlabs_TIM::TIM_Close(m_serialNo_TIM);

		Thorlabs_FF::FF_Close(m_serialNo_FF1);
		Thorlabs_FF::FF_StopPolling(m_serialNo_FF1);
		Thorlabs_FF::FF_Close(m_serialNo_FF2);
		Thorlabs_FF::FF_StopPolling(m_serialNo_FF2);

		Thorlabs_KDC::CC_StopPolling(m_serialNo_KDC);
		Thorlabs_KDC::CC_Close(m_serialNo_KDC);

		m_isConnected = false;
		m_isCompatible = false;
	}
	emit(connectedDevice(m_isConnected && m_isCompatible));
}

void NIDAQ::init() {
	calculateHomePositionBounds();

	elementPositionTimer = new QTimer();
	QMetaObject::Connection connection = QWidget::connect(elementPositionTimer, SIGNAL(timeout()), this, SLOT(getElements()));
}

void NIDAQ::setElement(DeviceElement element, int position) {
	switch ((DEVICE_ELEMENT)element.index) {
		case DEVICE_ELEMENT::CALFLIPMIRROR:
			setCalFlipMirror(position);
			break;
		case DEVICE_ELEMENT::BEAMBLOCK:
			setBeamBlock(position);
			break;
		case DEVICE_ELEMENT::MOVEMIRROR:
			setMirror(position);
			break;
		default:
			break;
	}
	getElement(element);
}

void NIDAQ::getElement(DeviceElement element) {
	int position{ -1 };
	switch ((DEVICE_ELEMENT)element.index) {
		case DEVICE_ELEMENT::CALFLIPMIRROR:
			position = Thorlabs_FF::FF_GetPosition(m_serialNo_FF1);
			break;
		case DEVICE_ELEMENT::BEAMBLOCK:
			position = Thorlabs_FF::FF_GetPosition(m_serialNo_FF2);
			break;
		case DEVICE_ELEMENT::MOVEMIRROR:
			//position = Thorlabs_FF::FF_GetPosition(m_serialNo_FF2);
			break;
		default:
			return;
	}
	emit(elementPositionChanged(element, position));
}

void NIDAQ::setElements(ScanControl::SCAN_PRESET preset) {
	int presetNr{ 0 };
	switch (preset) {
		case ScanControl::SCAN_BRIGHTFIELD:
		case ScanControl::SCAN_BRILLOUIN:
			presetNr = 0;
			break;
		case ScanControl::SCAN_CALIBRATION:
			presetNr = 1;
			break;
		case ScanControl::SCAN_ODT:
			presetNr = 2;
			break;
		default:
			return;
	}
	setCalFlipMirror(m_presets[presetNr][0]);
	setBeamBlock(m_presets[presetNr][1]);
	getElements();
}

void NIDAQ::getElements() {
	std::vector<int> elementPositions(m_deviceElements.deviceCount(), -1);
	elementPositions[0] = Thorlabs_FF::FF_GetPosition(m_serialNo_FF1);
	elementPositions[1] = Thorlabs_FF::FF_GetPosition(m_serialNo_FF2);
	emit(elementPositionsChanged(elementPositions));
}

void NIDAQ::setCalFlipMirror(int position) {
	Thorlabs_FF::FF_MoveToPosition(m_serialNo_FF1, (Thorlabs_FF::FF_Positions)position);
}

void NIDAQ::setBeamBlock(int position) {
	Thorlabs_FF::FF_MoveToPosition(m_serialNo_FF2, (Thorlabs_FF::FF_Positions)position);
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
	DWORD status = Thorlabs_KDC::CC_GetStatusBits(m_serialNo_KDC);
	while (status & (0x00000010 | 0x00000020)) {
		status = Thorlabs_KDC::CC_GetStatusBits(m_serialNo_KDC);
	}
	int currentPos = Thorlabs_KDC::CC_GetPosition(m_serialNo_KDC);
}

void NIDAQ::applyScanPosition() {
	// set the x- and y-position
	m_voltages = positionToVoltage(POINT2{ 1e-6*m_position.x, 1e-6*m_position.y });
	float64 data[2] = { m_voltages.Ux, m_voltages.Uy };
	DAQmxWriteAnalogF64(AOtaskHandle, 1, true, 10.0, DAQmx_Val_GroupByChannel, data, NULL, NULL);
	// set the z-position
	Thorlabs_TIM::TIM_MoveAbsolute(m_serialNo_TIM, m_channelPosZ, m_PiezoIncPerMum * m_position.z);
	calculateCurrentPositionBounds();
	announcePosition();
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

void NIDAQ::setVoltage(VOLTAGE2 voltages) {
	DAQmxStopTask(AOtaskHandle);
	float64 data[2] = { voltages.Ux, voltages.Uy };
	DAQmxWriteAnalogF64(AOtaskHandle, 1, true, 10.0, DAQmx_Val_GroupByChannel, data, NULL, NULL);

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

void NIDAQ::loadVoltagePositionCalibration(std::string filepath) {

	using namespace std::experimental::filesystem::v1;

	if (exists(filepath)) {
		H5::H5File file(&filepath[0], H5F_ACC_RDONLY);

		// read date
		H5::Group root = file.openGroup("/");
		H5::Attribute attr = root.openAttribute("date");
		H5::DataType type = attr.getDataType();
		hsize_t dateLength = attr.getStorageSize();
		char *buf = new char[dateLength + 1];
		attr.read(type, buf);
		m_calibration.date.assign(buf, dateLength);
		delete[] buf;
		buf = nullptr;

		// read calibration values
		m_calibration.translation.x = getCalibrationValue(file, "/translation/x");
		m_calibration.translation.y = getCalibrationValue(file, "/translation/y");
		m_calibration.rho = getCalibrationValue(file, "/rotation");
		m_calibration.coef.a = getCalibrationValue(file, "/coefficients/a");
		m_calibration.coef.b = getCalibrationValue(file, "/coefficients/b");
		m_calibration.coef.c = getCalibrationValue(file, "/coefficients/c");
		m_calibration.coef.d = getCalibrationValue(file, "/coefficients/d");

		m_calibration.bounds.xMin = getCalibrationValue(file, "/bounds/xMin");
		m_calibration.bounds.xMax = getCalibrationValue(file, "/bounds/xMax");
		m_calibration.bounds.yMin = getCalibrationValue(file, "/bounds/yMin");
		m_calibration.bounds.yMax = getCalibrationValue(file, "/bounds/yMax");
		m_absoluteBounds = m_calibration.bounds;
		m_calibration.valid = true;
	}
	centerPosition();
	calculateHomePositionBounds();
}

double NIDAQ::getCalibrationValue(H5::H5File file, std::string datasetName) {
	using namespace H5;
	double value{ 0 };

	DataSet dataset = file.openDataSet(datasetName.c_str());
	DataSpace filespace = dataset.getSpace();
	int rank = filespace.getSimpleExtentNdims();
	hsize_t dims[2];
	rank = filespace.getSimpleExtentDims(dims);
	DataSpace memspace(1, dims);
	dataset.read(&value, PredType::NATIVE_DOUBLE, memspace, filespace);

	return value;
}

void NIDAQ::triggerCamera() {
	DAQmxWriteDigitalLines(DOtaskHandle, 1, true, 10, DAQmx_Val_GroupByChannel, &m_TTL.low, NULL, NULL);
	DAQmxWriteDigitalLines(DOtaskHandle, 1, true, 10, DAQmx_Val_GroupByChannel, &m_TTL.high, NULL, NULL);
	DAQmxWriteDigitalLines(DOtaskHandle, 1, true, 10, DAQmx_Val_GroupByChannel, &m_TTL.low, NULL, NULL);
}

void NIDAQ::setAcquisitionVoltages(ACQ_VOLTAGES voltages) {
	// Stop DAQ tasks
	DAQmxStopTask(AOtaskHandle);
	DAQmxStopTask(DOtaskHandle);

	// Write analog voltages
	DAQmxWriteAnalogF64(AOtaskHandle, voltages.numberSamples, false, 10.0, DAQmx_Val_GroupByChannel, &voltages.mirror[0], NULL, NULL);
	// Write digital voltages
	DAQmxWriteDigitalLines(DOtaskHandle, voltages.numberSamples, false, 10, DAQmx_Val_GroupByChannel, &voltages.trigger[0], NULL, NULL);
	
	// Start DAQ tasks
	DAQmxStartTask(DOtaskHandle);
	DAQmxStartTask(AOtaskHandle);
}

POINT3 NIDAQ::getPosition() {
	return m_position;
}

void NIDAQ::centerPosition() {
	m_position = { 0, 0, 0 };
	// Set current position to zero
	Thorlabs_TIM::TIM_Home(m_serialNo_TIM, m_channelPosZ);
	// set the scan position
	applyScanPosition();
}
