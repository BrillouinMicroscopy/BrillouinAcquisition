#include "stdafx.h"
#include "Calibration.h"
#include "../../Devices/CalibrationHelper.h"
#include "../../simplemath.h"

Calibration::Calibration(QObject* parent, Acquisition* acquisition, Camera** camera, NIDAQ** nidaq)
	: AcquisitionMode(parent, acquisition), m_camera(camera), m_NIDAQ(nidaq) {
}

Calibration::~Calibration() {}

void Calibration::setCameraSetting(CAMERA_SETTING type, double value) {
	switch (type) {
		case CAMERA_SETTING::EXPOSURE:
			m_cameraSettings.exposureTime = value;
			break;
		case CAMERA_SETTING::GAIN:
			m_cameraSettings.gain = value;
			break;
	}

	emit(s_cameraSettingsChanged(m_cameraSettings));
}

void Calibration::init() {}

void Calibration::initialize() {}

void Calibration::startRepetitions() {
	bool allowed = m_acquisition->enableMode(ACQUISITION_MODE::SPATIALCALIBRATION);
	if (!allowed) {
		return;
	}

	// reset abort flag
	m_abort = false;

	// configure camera for measurement
	CAMERA_SETTINGS settings = (*m_camera)->getSettings();
	// set ROI and readout parameters to default Brillouin values, exposure time and gain will be kept
	settings.roi.left = 0;
	settings.roi.top = 0;
	settings.roi.width = m_calibration.cameraProperties.width;
	settings.roi.height = m_calibration.cameraProperties.height;
	settings.readout.pixelEncoding = L"Raw8";
	settings.readout.triggerMode = L"External";
	settings.readout.cycleMode = L"Continuous";
	settings.frameCount = m_acqSettings.Ux_steps * m_acqSettings.Uy_steps;

	(*m_camera)->startAcquisition(settings);

	// read back the applied settings
	m_cameraSettings = (*m_camera)->getSettings();

	// start repetition
	acquire();

	// configure camera for preview
	(*m_camera)->stopAcquisition();

	m_acquisition->disableMode(ACQUISITION_MODE::SPATIALCALIBRATION);
}

void Calibration::acquire(std::unique_ptr <StorageWrapper> & storage) {}

void Calibration::acquire() {
	m_status = ACQUISITION_STATUS::STARTED;
	emit(s_acquisitionStatus(m_status));

	// move to Brillouin configuration
	(*m_NIDAQ)->setPreset(SCAN_BRILLOUIN);

	// Create voltage vector
	std::vector<double> Ux = simplemath::linspace(m_acqSettings.Ux_min, m_acqSettings.Ux_max, m_acqSettings.Ux_steps);
	std::vector<double> Uy = simplemath::linspace(m_acqSettings.Uy_min, m_acqSettings.Uy_max, m_acqSettings.Uy_steps);
	m_acqSettings.voltages.clear();
	for (gsl::index i{ 0 }; i < Ux.size(); i++) {
		for (gsl::index j{ 0 }; j < Uy.size(); j++) {
			m_acqSettings.voltages.push_back({ Ux[i], Uy[j] });
		}
	}

	// Set first mirror voltage already
	(*m_NIDAQ)->setVoltage(m_acqSettings.voltages[0]);
	Sleep(100);

	std::vector<double> Ux_valid;
	std::vector<double> Uy_valid;
	std::vector<double> x_valid;
	std::vector<double> y_valid;

	ACQ_VOLTAGES voltages;

	/*
	 * Set the mirror voltage and trigger the camera
	 */
	// Construct the analog voltage vector and the trigger vector
	int samplesPerAngle{ 10 };
	int numberChannels{ 2 };
	voltages.numberSamples = m_acqSettings.Ux_steps * m_acqSettings.Uy_steps * samplesPerAngle;
	voltages.trigger = std::vector<uInt8>(voltages.numberSamples, 0);
	voltages.mirror = std::vector<float64>(voltages.numberSamples * numberChannels, 0);
	for (gsl::index i{ 0 }; i < m_acqSettings.Ux_steps * m_acqSettings.Uy_steps; i++) {
		voltages.trigger[i * samplesPerAngle + 2] = 1;
		voltages.trigger[i * samplesPerAngle + 3] = 1;
		std::fill_n(voltages.mirror.begin() + i * samplesPerAngle, samplesPerAngle, m_acqSettings.voltages[i].Ux);
		std::fill_n(voltages.mirror.begin() + i * samplesPerAngle + m_acqSettings.Ux_steps * m_acqSettings.Uy_steps * samplesPerAngle, samplesPerAngle, m_acqSettings.voltages[i].Uy);
	}
	// Apply voltages to NIDAQ board
	(*m_NIDAQ)->setAcquisitionVoltages(voltages);

	int rank_data{ 3 };
	hsize_t dims_data[3] = { 1, m_cameraSettings.roi.height, m_cameraSettings.roi.width };
	int bytesPerFrame = m_cameraSettings.roi.width * m_cameraSettings.roi.height;
	for (gsl::index i{ 0 }; i < m_acqSettings.Ux_steps * m_acqSettings.Uy_steps; i++) {

		// read images from camera
		std::vector<unsigned char> images(bytesPerFrame);

		for (gsl::index mm{ 0 }; mm < 1; mm++) {
			if (m_abort) {
				this->abortMode();
				return;
			}

			// acquire images
			int64_t pointerPos = (int64_t)bytesPerFrame * mm;
			(*m_camera)->getImageForAcquisition(&images[pointerPos], false);
		}

		// Extract spot position from camera image
		auto iterator_max = std::max_element(images.begin(), images.end());
		auto index = std::distance(images.begin(), iterator_max);
		if (*iterator_max > m_minimalIntensity) {
			int y = floor(index / m_cameraSettings.roi.height);
			int x = index % m_cameraSettings.roi.height;

			double x_m = m_calibration.cameraProperties.pixelSize / m_calibration.cameraProperties.mag
				* (x - m_calibration.cameraProperties.width / 2 - 0.5);
			double y_m = -1 * m_calibration.cameraProperties.pixelSize / m_calibration.cameraProperties.mag
				* (y - m_calibration.cameraProperties.height / 2 - 0.5);

			Ux_valid.push_back(m_acqSettings.voltages[i].Ux);
			Uy_valid.push_back(m_acqSettings.voltages[i].Uy);
			x_valid.push_back(x_m);
			y_valid.push_back(y_m);
		}
	}

	// Construct spatial calibration object
	m_calibration.date = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
		.toString(Qt::ISODateWithMs).toStdString();
	m_calibration.voltages.Ux = Ux_valid;
	m_calibration.voltages.Uy = Uy_valid;
	m_calibration.positions.x = x_valid;
	m_calibration.positions.y = y_valid;
	m_calibration.valid = true;

	CalibrationHelper::calculateCalibrationBounds(&m_calibration);
	CalibrationHelper::calculateCalibrationWeights(&m_calibration);

	(*m_NIDAQ)->setSpatialCalibration(m_calibration);

	m_status = ACQUISITION_STATUS::FINISHED;
	emit(s_acquisitionStatus(m_status));
}

void Calibration::abortMode(std::unique_ptr <StorageWrapper> & storage) {}

void Calibration::loadVoltagePositionCalibration(std::string filepath) {

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

		// Read calibration maps
		m_calibration.positions.x = getCalibrationMap(file, "/maps/positions/x");
		m_calibration.positions.y = getCalibrationMap(file, "/maps/positions/y");
		m_calibration.voltages.Ux = getCalibrationMap(file, "/maps/voltages/Ux");
		m_calibration.voltages.Uy = getCalibrationMap(file, "/maps/voltages/Uy");

		m_calibration.cameraProperties.width = getCalibrationValue(file, "/camera/width");
		m_calibration.cameraProperties.height = getCalibrationValue(file, "/camera/height");
		m_calibration.cameraProperties.pixelSize = getCalibrationValue(file, "/camera/pixelSize");
		m_calibration.cameraProperties.mag = getCalibrationValue(file, "/camera/magnification");

		CalibrationHelper::calculateCalibrationBounds(&m_calibration);
		CalibrationHelper::calculateCalibrationWeights(&m_calibration);

		m_calibration.valid = true;
	}

	(*m_NIDAQ)->setSpatialCalibration(m_calibration);
}

double Calibration::getCalibrationValue(H5::H5File file, std::string datasetName) {
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

std::vector<double> Calibration::getCalibrationMap(H5::H5File file, std::string datasetName) {
	using namespace H5;
	std::vector<double> map;

	DataSet dataset = file.openDataSet(datasetName.c_str());
	DataSpace filespace = dataset.getSpace();
	int rank = filespace.getSimpleExtentNdims();
	hsize_t dims[2];
	rank = filespace.getSimpleExtentDims(dims);
	DataSpace memspace(2, dims);
	map.resize(dims[0] * dims[1]);
	dataset.read(&map[0], PredType::NATIVE_DOUBLE, memspace, filespace);

	return map;
}

void Calibration::abortMode() {
	m_acquisition->disableMode(ACQUISITION_MODE::SPATIALCALIBRATION);
	m_status = ACQUISITION_STATUS::ABORTED;
	emit(s_acquisitionStatus(m_status));
}
