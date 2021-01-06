#include "stdafx.h"
#include "filesystem"
#include "ScaleCalibration.h"

/*
 * Public definitions
 */

ScaleCalibration::ScaleCalibration(QObject* parent, Acquisition* acquisition, Camera** camera, ScanControl** scanControl)
	: AcquisitionMode(parent, acquisition, scanControl), m_camera(camera) {
}

ScaleCalibration::~ScaleCalibration() {}

/*
 * Public slots
 */

void ScaleCalibration::startRepetitions() {
	bool allowed = m_acquisition->enableMode(ACQUISITION_MODE::SCALECALIBRATION);
	if (!allowed) {
		return;
	}

	// reset abort flag
	m_abort = false;

	// start repetition
	acquire();

	// configure camera for preview

	m_acquisition->disableMode(ACQUISITION_MODE::SCALECALIBRATION);
}

void ScaleCalibration::load(std::string filepath) {

	using namespace std::filesystem;

	if (exists(filepath)) {
		auto file = H5::H5File(&filepath[0], H5F_ACC_RDONLY);

		// read date
		auto root = file.openGroup("/");

		m_scaleCalibration.originPix = readPoint(root, "origin");

		m_scaleCalibration.pixToMicrometerX = readPoint(root, "pixToMicrometerX");
		m_scaleCalibration.pixToMicrometerY = readPoint(root, "pixToMicrometerY");

		m_scaleCalibration.micrometerToPixX = readPoint(root, "micrometerToPixX");
		m_scaleCalibration.micrometerToPixY = readPoint(root, "micrometerToPixY");

		// Apply the scale calibration
		(*m_scanControl)->setScaleCalibration(m_scaleCalibration);
	}
}

/*
 * Private definitions
 */

void ScaleCalibration::abortMode(std::unique_ptr <StorageWrapper>& storage) {}

void ScaleCalibration::abortMode() {
	m_acquisition->disableMode(ACQUISITION_MODE::SCALECALIBRATION);
	setAcquisitionStatus(ACQUISITION_STATUS::ABORTED);
}

void ScaleCalibration::save() {
	/*
	 * Construct the filepath
	 */
	auto folder = m_acquisition->getCurrentFolder();

	auto fulldate = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
		.toString(Qt::ISODateWithMs).toStdString();

	auto shortdate = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
		.toString("yyyy-MM-ddTHHmmss").toStdString();
	auto dt1 = QDateTime::currentDateTime();
	auto dt2 = dt1.toUTC();
	dt1.setTimeSpec(Qt::UTC);

	auto offset = dt2.secsTo(dt1) / 3600;

	auto offse = QString("+%1").arg(offset, 2, 10, QChar('0')).toStdString();

	auto filepath{ folder + "/_scaleCalibration_" };
	filepath += shortdate + offse + ".h5";

	/*
	 * Open file for writing, overwrite existing file
	 */
	auto file = H5::H5File(&filepath[0], H5F_ACC_TRUNC);

	// write date
	// Create the data space for the attribute.
	auto attr_dataspace = H5::DataSpace(H5S_SCALAR);
	// Create new string datatype for attribute
	auto strdatatype = H5::StrType(H5::PredType::C_S1, fulldate.size());
	auto root = file.openGroup("/");
	auto attr = root.createAttribute("date", strdatatype, attr_dataspace);
	auto type = attr.getDataType();
	attr.write(strdatatype, &fulldate[0]);

	writePoint(root, "origin", m_scaleCalibration.originPix);

	writePoint(root, "pixToMicrometerX", m_scaleCalibration.pixToMicrometerX);
	writePoint(root, "pixToMicrometerY", m_scaleCalibration.pixToMicrometerY);

	writePoint(root, "micrometerToPixX", m_scaleCalibration.micrometerToPixX);
	writePoint(root, "micrometerToPixY", m_scaleCalibration.micrometerToPixY);
}

/*
 * Private slots
 */

void ScaleCalibration::acquire(std::unique_ptr <StorageWrapper>& storage) {}

void ScaleCalibration::acquire() {
	setAcquisitionStatus(ACQUISITION_STATUS::STARTED);

	auto image0 = cv::InputArray{};
	auto image1 = cv::InputArray{};

	auto shift = cv::phaseCorrelate(image0, image1);

	save();

	setAcquisitionStatus(ACQUISITION_STATUS::FINISHED);
}

void ScaleCalibration::writePoint(H5::Group group, std::string name, POINT2 point) {
	using namespace H5;

	auto subgroup = H5::Group(group.createGroup(&name[0]));

	hsize_t dims[2];
	dims[0] = 1;
	dims[1] = 1;
	auto dataspace = DataSpace(2, dims);
	
	// Write x coordinate
	auto x = subgroup.createAttribute("x", PredType::NATIVE_DOUBLE, dataspace);
	x.write(PredType::NATIVE_DOUBLE, &point.x);

	// Write y coordinate
	auto y = subgroup.createAttribute("y", PredType::NATIVE_DOUBLE, dataspace);
	y.write(PredType::NATIVE_DOUBLE, &point.y);
}

POINT2 ScaleCalibration::readPoint(H5::Group group, const std::string& name) {
	auto point = POINT2{};

	// Open point group
	try {
		auto pointGroup = group.openGroup(&name[0]);

		// Read x coordinate
		auto attr = pointGroup.openAttribute("x");
		auto type = attr.getDataType();
		attr.read(type, &point.x);

		// Read y coordinate
		attr = pointGroup.openAttribute("y");
		type = attr.getDataType();
		attr.read(type, &point.y);

	} catch (H5::GroupIException exception) {
		// Exception
	}

	return point;
}