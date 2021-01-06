#include "stdafx.h"
#include "filesystem"
#include "ScaleCalibration.h"

#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"

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

	(*m_scanControl)->setPosition(m_startPosition);

	QMetaObject::invokeMethod(
		(*m_scanControl),
		[&m_scanControl = (*m_scanControl)]() { m_scanControl->startAnnouncing(); },
		Qt::AutoConnection
	);

	setAcquisitionStatus(ACQUISITION_STATUS::ABORTED);
}

void ScaleCalibration::save(std::vector<std::vector<unsigned char>> images, double dx, double dy) {
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

	QMetaObject::invokeMethod(
		(*m_scanControl),
		[&m_scanControl = (*m_scanControl)]() { m_scanControl->stopAnnouncing(); },
		Qt::AutoConnection
	);
	// Set optical elements for brightfield/Brillouin imaging
	(*m_scanControl)->setPreset(ScanPreset::SCAN_BRIGHTFIELD);
	Sleep(500);

	// Get the current stage position
	m_startPosition = (*m_scanControl)->getPosition();

	/*
	 * Configure the camera
	 */
	auto cameraSettings = (*m_camera)->getSettings();

	cameraSettings.roi.left = 1000;
	cameraSettings.roi.top = 800;
	cameraSettings.roi.width_physical = 1000;
	cameraSettings.roi.height_physical = 1000;
	cameraSettings.frameCount = 1;
	(*m_camera)->setSettings(cameraSettings);

	cameraSettings = (*m_camera)->getSettings();

	// This will fail if the camera uses a format other than unsigned char.
	// Should be moved to the camera into cameraSettings.roi.bytesPerFrame.
	auto bytesPerFrame = cameraSettings.roi.width_binned * cameraSettings.roi.height_binned;

	/*
	 * We acquire three images here, one at the origin, and one each shifted in x- and y-direction.
	 */
	auto dx{ 10.0 };					// [�m]	shift in x-direction
	auto dy{ 10.0 };					// [�m]	shift in y-direction
	auto hysteresisCompensation{ 10.0 };// [�m] distance for compensation of the stage hysteresis
	// Construct the positions
	auto positions = std::vector<POINT2>{ { 0, 0 }, { dx, 0 }, { 0, dy } };

	// Acquire memory for image acquisition
	auto images = std::vector<std::vector<unsigned char>>(positions.size());
	for (auto& image : images) {
		image.resize(bytesPerFrame);
	}

	(*m_camera)->startAcquisition(cameraSettings);

	auto iteration{ 0 };
	for (const auto& position : positions) {
		// Abort if requested
		if (m_abort) {
			this->abortMode();
			return;
		}

		/*
		 * Set the new stage position
		 */
		auto positionAbsolute = m_startPosition + POINT3{ position.x, position.y, 0 };
		// To prevent problems with the hysteresis of the stage, we always move to the desired point coming from lower values.
		(*m_scanControl)->setPosition(positionAbsolute - POINT3{ hysteresisCompensation, hysteresisCompensation, 0 });
		Sleep(100);
		(*m_scanControl)->setPosition(positionAbsolute);
		Sleep(200);

		/*
		 * Acquire the camera image
		 */

		// acquire images
		(*m_camera)->getImageForAcquisition(&(images[iteration])[0], false);

		// Sometimes the uEye camera returns a black image (only zeros), we try to catch this here by
		// repeating the acquisition a maximum of 5 times
		auto sum = simplemath::sum(images[iteration]);
		auto tryCount{ 0 };
		while (sum == 0 && 5 > tryCount++) {
			(*m_camera)->getImageForAcquisition(&(images[iteration])[0], false);

			sum = simplemath::sum(images[iteration]);
		}

		++iteration;
	}

	// Stop the camera acquisition
	(*m_camera)->stopAcquisition();

	// Create input matrices for OpenCV from camera images
	auto imagesCV = std::vector<cv::Mat>(images.size());
	for (gsl::index i{ 0 }; i < images.size(); i++) {
		imagesCV[i] = cv::Mat(cameraSettings.roi.height_binned, cameraSettings.roi.width_binned, CV_8UC1, &(images[i])[0]);
	}

	/*
	 * Convert the images to floating point (CV_32FC1)
	 * for phase correlation (phaseCorrelate doesn't throw an error
	 * on CV_8UC1 data, it just doesn't work).
	 */
	auto imagesCV_float = std::vector<cv::Mat>(imagesCV.size());
	for (gsl::index i{ 0 }; i < imagesCV.size(); i++) {
		imagesCV[i].convertTo(imagesCV_float[i], CV_32FC1, 1.0 / 255);
	}

	/*
	 * Determine the shift in pixels
	 */

	// Create a hanning window to reduce edge effects
	auto hanningWindow = cv::Mat{};
	cv::createHanningWindow(hanningWindow, imagesCV_float[0].size(), CV_32F);

	// Show the input images for debugging
	//cv::imshow("Origin", imagesCV_float[0]);
	//cv::imshow("Dx", imagesCV_float[1]);
	//cv::imshow("Dy", imagesCV_float[2]);
	//cv::imshow("Hanning window", hanningWindow);

	// Calculate the shift
	auto shiftDx = cv::phaseCorrelate(imagesCV_float[0], imagesCV_float[1], hanningWindow);
	auto shiftDy = cv::phaseCorrelate(imagesCV_float[0], imagesCV_float[2], hanningWindow);

	/*
	 * Construct the scale calibration
	 */
	m_scaleCalibration.micrometerToPixX = { -1 * shiftDx.x / dx, shiftDx.y / dx };
	m_scaleCalibration.micrometerToPixY = { -1 * shiftDy.x / dy, shiftDy.y / dy };
	ScaleCalibrationHelper::initializeCalibrationFromMicrometer(&m_scaleCalibration);

	// Store the calibration in a file
	save(images, dx, dy);

	// Apply the scale calibration
	// TODO: Check if it's valid before applying
	//(*m_scanControl)->setScaleCalibration(m_scaleCalibration);

	/*
	 * Cleanup the acquisition mode
	 */
	(*m_scanControl)->setPosition(m_startPosition);
	QMetaObject::invokeMethod(
		(*m_scanControl),
		[&m_scanControl = (*m_scanControl)]() { m_scanControl->startAnnouncing(); },
		Qt::AutoConnection
	);

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