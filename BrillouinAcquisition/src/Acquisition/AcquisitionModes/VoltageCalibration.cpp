#include "stdafx.h"
#include "filesystem"
#include "VoltageCalibration.h"
#include "../../simplemath.h"

/*
 * Public definitions
 */

VoltageCalibration::VoltageCalibration(QObject* parent, Acquisition* acquisition, Camera** camera, ODTControl** ODTControl)
	: AcquisitionMode(parent, acquisition), m_camera(camera), m_ODTControl(ODTControl) {
}

VoltageCalibration::~VoltageCalibration() {}

/*
 * Public slots
 */

void VoltageCalibration::startRepetitions() {
	bool allowed = m_acquisition->enableMode(ACQUISITION_MODE::VOLTAGECALIBRATION);
	if (!allowed) {
		return;
	}

	// reset abort flag
	m_abort = false;

	CAMERA_OPTIONS cameraOptions = (*m_camera)->getOptions();

	// configure camera for measurement
	m_cameraSettings = (*m_camera)->getSettings();
	// set ROI and readout parameters to default Brillouin values, exposure time and gain will be kept
	if (m_cameraSettings.exposureTime > 0.003) {
		m_cameraSettings.exposureTime = 0.003;
	}
	m_cameraSettings.roi.left = 0;
	m_cameraSettings.roi.top = 0;
	// Set camera to full frame
	m_cameraSettings.roi.width_physical = cameraOptions.ROIWidthLimits[1];
	m_cameraSettings.roi.height_physical = cameraOptions.ROIHeightLimits[1];
	m_cameraSettings.readout.pixelEncoding = L"Raw8";
	m_cameraSettings.readout.triggerMode = L"External";
	m_cameraSettings.readout.cycleMode = L"Continuous";
	m_cameraSettings.frameCount = (long long)m_acqSettings.Ux_steps * m_acqSettings.Uy_steps;

	// start repetition
	acquire();

	// configure camera for preview

	m_acquisition->disableMode(ACQUISITION_MODE::VOLTAGECALIBRATION);
}

void VoltageCalibration::setCameraSetting(CAMERA_SETTING type, double value) {
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

void VoltageCalibration::load(std::string filepath) {

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
		m_voltageCalibration.date.assign(buf, dateLength);
		delete[] buf;
		buf = nullptr;

		// Read calibration maps
		m_voltageCalibration.positions.x = readCalibrationMap(file, "/maps/positions/x");
		m_voltageCalibration.positions.y = readCalibrationMap(file, "/maps/positions/y");
		m_voltageCalibration.voltages.Ux = readCalibrationMap(file, "/maps/voltages/Ux");
		m_voltageCalibration.voltages.Uy = readCalibrationMap(file, "/maps/voltages/Uy");

		m_voltageCalibration.valid = true;
	}

	VoltageCalibrationHelper::calculateCalibrationWeights(&m_voltageCalibration);

	(*m_ODTControl)->setVoltageCalibration(m_voltageCalibration);
}

/*
 * Private definitions
 */

void VoltageCalibration::abortMode(std::unique_ptr <StorageWrapper>& storage) {}

void VoltageCalibration::abortMode() {
	m_acquisition->disableMode(ACQUISITION_MODE::VOLTAGECALIBRATION);
	setAcquisitionStatus(ACQUISITION_STATUS::ABORTED);
}

double VoltageCalibration::readCalibrationValue(H5::H5File file, std::string datasetName) {
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

void VoltageCalibration::writeCalibrationValue(H5::Group group, const H5std_string datasetName, double value) {
	using namespace H5;

	hsize_t dims[2];
	dims[0] = 1;
	dims[1] = 1;

	DataSpace dataspace = DataSpace(2, dims);
	DataSet dataset = group.createDataSet(&datasetName[0], PredType::NATIVE_DOUBLE, dataspace);

	dataset.write(&value, PredType::NATIVE_DOUBLE);
}

std::vector<double> VoltageCalibration::readCalibrationMap(H5::H5File file, std::string datasetName) {
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

void VoltageCalibration::writeCalibrationMap(H5::Group group, std::string datasetName, std::vector<double> map) {
	using namespace H5;

	hsize_t dims[2];
	dims[0] = 1;
	dims[1] = map.size();

	DataSpace dataspace = DataSpace(2, dims);
	DataSet dataset = group.createDataSet(&datasetName[0], PredType::NATIVE_DOUBLE, dataspace);

	dataset.write(&map[0], PredType::NATIVE_DOUBLE);

}

void VoltageCalibration::save() {

	if (m_voltageCalibration.positions.x.size() == 0) {
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

	H5::Group maps(file.createGroup("/maps"));

	H5::Group group_positions(maps.createGroup("positions"));
	writeCalibrationMap(group_positions, "x", m_voltageCalibration.positions.x);
	writeCalibrationMap(group_positions, "y", m_voltageCalibration.positions.y);

	H5::Group group_voltages(maps.createGroup("voltages"));
	writeCalibrationMap(group_voltages, "Ux", m_voltageCalibration.voltages.Ux);
	writeCalibrationMap(group_voltages, "Uy", m_voltageCalibration.voltages.Uy);
}

/*
 * Private slots
 */

void VoltageCalibration::acquire(std::unique_ptr <StorageWrapper> & storage) {}

void VoltageCalibration::acquire() {
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
		hsize_t dims_data[3] = { 1, (hsize_t)m_cameraSettings.roi.height_binned, (hsize_t)m_cameraSettings.roi.width_binned };
		int bytesPerFrame = m_cameraSettings.roi.width_binned * m_cameraSettings.roi.height_binned;
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
				int y = m_cameraSettings.roi.height_binned - floor(index / m_cameraSettings.roi.width_binned);
				int x = index % m_cameraSettings.roi.width_binned;
				
				POINT2 pos = (*m_ODTControl)->pixToMicroMeter({ (double)x, (double)y });

				Ux_valid.push_back(m_acqSettings.voltages[i + chunkBegin].Ux);
				Uy_valid.push_back(m_acqSettings.voltages[i + chunkBegin].Uy);
				x_valid.push_back(pos.x);
				y_valid.push_back(pos.y);
			}
		}

		(*m_camera)->stopAcquisition();
	}

	// Construct spatial calibration object
	m_voltageCalibration.date = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
		.toString(Qt::ISODateWithMs).toStdString();
	m_voltageCalibration.voltages.Ux = Ux_valid;
	m_voltageCalibration.voltages.Uy = Uy_valid;
	m_voltageCalibration.positions.x = x_valid;
	m_voltageCalibration.positions.y = y_valid;
	m_voltageCalibration.valid = true;

	VoltageCalibrationHelper::calculateCalibrationWeights(&m_voltageCalibration);

	save();
	(*m_ODTControl)->setVoltageCalibration(m_voltageCalibration);

	(*m_ODTControl)->setPreset(ScanPreset::SCAN_BRILLOUIN);

	setAcquisitionStatus(ACQUISITION_STATUS::FINISHED);
}