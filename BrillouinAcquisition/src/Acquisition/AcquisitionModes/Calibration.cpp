#include "stdafx.h"
#include "H5Cpp.h"
#include "Calibration.h"
#include "../../Devices/CalibrationHelper.h"
#include "../../simplemath.h"

/*
 * Public definitions
 */

Calibration::Calibration(QObject* parent, Acquisition* acquisition, Camera** camera, ODTControl** ODTControl)
	: AcquisitionMode(parent, acquisition), m_camera(camera), m_ODTControl(ODTControl) {
}

Calibration::~Calibration() {}

/*
 * Public slots
 */

void Calibration::startRepetitions() {
	bool allowed = m_acquisition->enableMode(ACQUISITION_MODE::SPATIALCALIBRATION);
	if (!allowed) {
		return;
	}

	// reset abort flag
	m_abort = false;

	// configure camera for measurement
	m_cameraSettings = (*m_camera)->getSettings();
	// set ROI and readout parameters to default Brillouin values, exposure time and gain will be kept
	if (m_cameraSettings.exposureTime > 0.003) {
		m_cameraSettings.exposureTime = 0.003;
	}
	m_cameraSettings.roi.left = 0;
	m_cameraSettings.roi.top = 0;
	m_cameraSettings.roi.width = m_calibration.microscopeProperties.width;
	m_cameraSettings.roi.height = m_calibration.microscopeProperties.height;
	m_cameraSettings.readout.pixelEncoding = L"Raw8";
	m_cameraSettings.readout.triggerMode = L"External";
	m_cameraSettings.readout.cycleMode = L"Continuous";
	m_cameraSettings.frameCount = (long long)m_acqSettings.Ux_steps * m_acqSettings.Uy_steps;

	// start repetition
	acquire();

	// configure camera for preview

	m_acquisition->disableMode(ACQUISITION_MODE::SPATIALCALIBRATION);
}

void Calibration::initialize() {
	emit(calibrationChanged(m_calibration));
}

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

void Calibration::load(std::string filepath) {

	using namespace std::filesystem;

	if (exists(filepath)) {
		H5::H5File file(&filepath[0], H5F_ACC_RDONLY);

		// read date
		H5::Group root = file.openGroup("/");
		H5::Attribute attr = root.openAttribute("date");
		H5::DataType type = attr.getDataType();
		hsize_t dateLength = attr.getStorageSize();
		char* buf = new char[dateLength + 1];
		attr.read(type, buf);
		m_calibration.date.assign(buf, dateLength);
		delete[] buf;
		buf = nullptr;

		// Read calibration maps
		m_calibration.positions.x = readCalibrationMap(file, "/maps/positions/x");
		m_calibration.positions.y = readCalibrationMap(file, "/maps/positions/y");
		m_calibration.voltages.Ux = readCalibrationMap(file, "/maps/voltages/Ux");
		m_calibration.voltages.Uy = readCalibrationMap(file, "/maps/voltages/Uy");

		m_calibration.microscopeProperties.width = readCalibrationValue(file, "/camera/width");
		m_calibration.microscopeProperties.height = readCalibrationValue(file, "/camera/height");
		m_calibration.microscopeProperties.pixelSize = readCalibrationValue(file, "/camera/pixelSize");
		m_calibration.microscopeProperties.mag = readCalibrationValue(file, "/camera/magnification");


		m_calibration.valid = true;
	}

	CalibrationHelper::calculateCalibrationBounds(&m_calibration);
	CalibrationHelper::calculateCalibrationWeights(&m_calibration);

	(*m_ODTControl)->setSpatialCalibration(m_calibration);

	emit(calibrationChanged(m_calibration));
}

void Calibration::setWidth(int width) {
	m_calibration.microscopeProperties.width = width;
	m_calibration.valid = false;
}

void Calibration::setHeight(int height) {
	m_calibration.microscopeProperties.height = height;
	m_calibration.valid = false;
}

void Calibration::setMagnification(double mag) {
	m_calibration.microscopeProperties.mag = mag;
	m_calibration.valid = false;
}

void Calibration::setPixelSize(double pixelSize) {
	m_calibration.microscopeProperties.pixelSize = pixelSize;
	m_calibration.valid = false;
}

/*
 * Private definitions
 */

void Calibration::abortMode(std::unique_ptr <StorageWrapper>& storage) {}

void Calibration::abortMode() {
	m_acquisition->disableMode(ACQUISITION_MODE::SPATIALCALIBRATION);
	setAcquisitionStatus(ACQUISITION_STATUS::ABORTED);
}

double Calibration::readCalibrationValue(H5::H5File file, std::string datasetName) {
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

void Calibration::writeCalibrationValue(H5::Group group, const H5std_string datasetName, double value) {
	using namespace H5;

	hsize_t dims[2];
	dims[0] = 1;
	dims[1] = 1;

	DataSpace dataspace = DataSpace(2, dims);
	DataSet dataset = group.createDataSet(&datasetName[0], PredType::NATIVE_DOUBLE, dataspace);

	dataset.write(&value, PredType::NATIVE_DOUBLE);
}

std::vector<double> Calibration::readCalibrationMap(H5::H5File file, std::string datasetName) {
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

void Calibration::writeCalibrationMap(H5::Group group, std::string datasetName, std::vector<double> map) {
	using namespace H5;

	hsize_t dims[2];
	dims[0] = 1;
	dims[1] = map.size();

	DataSpace dataspace = DataSpace(2, dims);
	DataSet dataset = group.createDataSet(&datasetName[0], PredType::NATIVE_DOUBLE, dataspace);

	dataset.write(&map[0], PredType::NATIVE_DOUBLE);

}

void Calibration::save() {

	if (m_calibration.positions.x.size() == 0) {
		return;
	}

	std::string folder = m_acquisition->getCurrentFolder();

	std::string fulldate = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
		.toString(Qt::ISODateWithMs).toStdString();

	std::string shortdate = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
		.toString("yyyy-MM-ddTHHmmss").toStdString();
	QDateTime dt1 = QDateTime::currentDateTime();
	QDateTime dt2 = dt1.toUTC();
	dt1.setTimeSpec(Qt::UTC);

	int offset = dt2.secsTo(dt1) / 3600;

	std::string offse = QString("+%1").arg(offset, 2, 10, QChar('0')).toStdString();

	std::string filepath{ folder + "/_positionCalibration_" };
	filepath += shortdate + offse + ".h5";


	H5::H5File file(&filepath[0], H5F_ACC_TRUNC);

	// write date
	// Create the data space for the attribute.
	H5::DataSpace attr_dataspace = H5::DataSpace(H5S_SCALAR);
	// Create new string datatype for attribute
	H5::StrType strdatatype(H5::PredType::C_S1, fulldate.size());
	H5::Group root = file.openGroup("/");
	H5::Attribute attr = root.createAttribute("date", strdatatype, attr_dataspace);
	H5::DataType type = attr.getDataType();
	attr.write(strdatatype, &fulldate[0]);

	H5::Group group_camera(file.createGroup("/camera"));
	writeCalibrationValue(group_camera, "width", m_calibration.microscopeProperties.width);
	writeCalibrationValue(group_camera, "height", m_calibration.microscopeProperties.height);
	writeCalibrationValue(group_camera, "pixelSize", m_calibration.microscopeProperties.pixelSize);
	writeCalibrationValue(group_camera, "magnification", m_calibration.microscopeProperties.mag);

	H5::Group maps(file.createGroup("/maps"));

	H5::Group group_positions(maps.createGroup("positions"));
	writeCalibrationMap(group_positions, "x", m_calibration.positions.x);
	writeCalibrationMap(group_positions, "y", m_calibration.positions.y);

	H5::Group group_voltages(maps.createGroup("voltages"));
	writeCalibrationMap(group_voltages, "Ux", m_calibration.voltages.Ux);
	writeCalibrationMap(group_voltages, "Uy", m_calibration.voltages.Uy);
}

/*
 * Private slots
 */

void Calibration::acquire(std::unique_ptr <StorageWrapper> & storage) {}

void Calibration::acquire() {
	setAcquisitionStatus(ACQUISITION_STATUS::STARTED);

	// move to Brillouin configuration
	(*m_ODTControl)->setPreset(ScanPreset::SCAN_BRILLOUIN);
	(*m_ODTControl)->setLEDLamp(false);

	std::vector<double> Ux_valid;
	std::vector<double> Uy_valid;
	std::vector<double> x_valid;
	std::vector<double> y_valid;

	// Create voltage vector
	std::vector<double> Ux = simplemath::linspace(m_acqSettings.Ux_min, m_acqSettings.Ux_max, m_acqSettings.Ux_steps);
	std::vector<double> Uy = simplemath::linspace(m_acqSettings.Uy_min, m_acqSettings.Uy_max, m_acqSettings.Uy_steps);
	m_acqSettings.voltages.clear();
	for (gsl::index i{ 0 }; i < Ux.size(); i++) {
		for (gsl::index j{ 0 }; j < Uy.size(); j++) {
			m_acqSettings.voltages.push_back({ Ux[i], Uy[j] });
		}
	}

	// We splite the requested amount of calibration positions into chunks of maximum 100 images,
	// since the PointGrey camera can't handle more without completely hanging.
	int nrImages{ 100 };
	int nrChunks = ceil(m_acqSettings.voltages.size() / double(nrImages));

	for (gsl::index chunk{ 0 }; chunk < nrChunks; chunk++) {

		(*m_camera)->startAcquisition(m_cameraSettings);

		int chunkBegin = chunk * nrImages;
		int chunkEnd = (chunk + 1) * nrImages - 1;
		if (chunkEnd > m_acqSettings.voltages.size() - 1) {
			chunkEnd = m_acqSettings.voltages.size() - 1;
		}
		int chunkSize = chunkEnd - chunkBegin + 1;

		// Set first mirror voltage already
		(*m_ODTControl)->setVoltage(m_acqSettings.voltages[chunkBegin]);
		Sleep(100);

		ACQ_VOLTAGES voltages;

		/*
			* Set the mirror voltage and trigger the camera
			*/
			// Construct the analog voltage vector and the trigger vector
		int samplesPerAngle{ 10 };
		int numberChannels{ 2 };
		voltages.numberSamples = chunkSize * samplesPerAngle;
		voltages.trigger = std::vector<uInt8>(voltages.numberSamples, 0);
		voltages.mirror = std::vector<float64>((float64)voltages.numberSamples * numberChannels, 0);
		for (gsl::index i{ 0 }; i < chunkSize; i++) {
			voltages.trigger[i * samplesPerAngle + 2] = 1;
			voltages.trigger[i * samplesPerAngle + 3] = 1;
			std::fill_n(voltages.mirror.begin() + i * samplesPerAngle, samplesPerAngle, m_acqSettings.voltages[i + chunkBegin].Ux);
			std::fill_n(voltages.mirror.begin() + i * samplesPerAngle + (size_t)chunkSize * samplesPerAngle, samplesPerAngle, m_acqSettings.voltages[i + chunkBegin].Uy);
		}
		// Apply voltages to NIDAQ board
		(*m_ODTControl)->setAcquisitionVoltages(voltages);

		int rank_data{ 3 };
		hsize_t dims_data[3] = { 1, (hsize_t)m_cameraSettings.roi.height, (hsize_t)m_cameraSettings.roi.width };
		int bytesPerFrame = m_cameraSettings.roi.width * m_cameraSettings.roi.height;
		for (gsl::index i{ 0 }; i < chunkSize; i++) {

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
				int y = floor(index / m_cameraSettings.roi.width);
				int x = index % m_cameraSettings.roi.width;

				double x_m = m_calibration.microscopeProperties.pixelSize / m_calibration.microscopeProperties.mag
					* ((double)x - m_calibration.microscopeProperties.width / 2 - 0.5);
				double y_m = -1 * m_calibration.microscopeProperties.pixelSize / m_calibration.microscopeProperties.mag
					* ((double)y - m_calibration.microscopeProperties.height / 2 - 0.5);

				Ux_valid.push_back(m_acqSettings.voltages[i + chunkBegin].Ux);
				Uy_valid.push_back(m_acqSettings.voltages[i + chunkBegin].Uy);
				x_valid.push_back(x_m);
				y_valid.push_back(y_m);
			}
		}

		(*m_camera)->stopAcquisition();
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

	save();
	(*m_ODTControl)->setSpatialCalibration(m_calibration);

	(*m_ODTControl)->setPreset(ScanPreset::SCAN_BRILLOUIN);

	emit(calibrationChanged(m_calibration));

	setAcquisitionStatus(ACQUISITION_STATUS::FINISHED);
}