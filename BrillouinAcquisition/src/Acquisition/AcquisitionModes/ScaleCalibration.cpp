#include "stdafx.h"
#include "filesystem"
#include "ScaleCalibration.h"

#include "src/helper/h5_helper.h"

#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"

#include <chrono>
#include <thread>

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

	// Check that we have camera and scancontrol
	if (!(*m_camera)) {
		return;
	}
	if (!(*m_scanControl)) {
		return;
	}

	/*
	 * Configure the camera
	 */
	m_cameraSettings = (*m_camera)->getSettings();

	m_cameraSettings.roi.left = 1000;
	m_cameraSettings.roi.top = 800;
	m_cameraSettings.roi.width_physical = 1000;
	m_cameraSettings.roi.height_physical = 1000;
	m_cameraSettings.frameCount = 1;
	(*m_camera)->setSettings(m_cameraSettings);

	m_cameraSettings = (*m_camera)->getSettings();

	acquire();

	m_acquisition->disableMode(ACQUISITION_MODE::SCALECALIBRATION);
	
}

void ScaleCalibration::load(std::string filepath) {

	using namespace std::filesystem;

	if (exists(filepath)) {
		try {
			auto file = H5::H5File(&filepath[0], H5F_ACC_RDONLY);

			// read date
			auto root = file.openGroup("/");

			m_scaleCalibration.originPix = readPoint(root, "origin");

			m_scaleCalibration.pixToMicrometerX = readPoint(root, "pixToMicrometerX");
			m_scaleCalibration.pixToMicrometerY = readPoint(root, "pixToMicrometerY");

			m_scaleCalibration.micrometerToPixX = readPoint(root, "micrometerToPixX");
			m_scaleCalibration.micrometerToPixY = readPoint(root, "micrometerToPixY");

			// Apply the scale calibration
			apply();
		} catch (H5::Exception& exception) {
			emit(s_scaleCalibrationStatus("Could not load the scale calibration", "Please select a valid scale calibration file."));
		}
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

template <typename T>
void ScaleCalibration::save(std::vector<std::vector<T>> images, std::vector<POINT2> positions) {
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

	try {
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

		// Write images and set positions as attributes
		auto imageGroup = root.createGroup("images");
		hsize_t dims[3] = {
			(hsize_t)m_cameraSettings.frameCount,
			(hsize_t)m_cameraSettings.roi.height_binned,
			(hsize_t)m_cameraSettings.roi.width_binned
		};
		auto names = std::vector<std::string>{ "origin", "dx", "dy" };
		auto i = gsl::index{ 0 };
		for (const auto& image : images) {
			// Check that we don't run into trouble iterating over two arrays
			if (i >= names.size()) {
				continue;
			}
			auto dataspace = H5::DataSpace(3, dims, dims);
			auto type_id = h5_helper::get_memtype<T>();
			// For some unknown reason using the overloaded function createDataSet(const H5std_string&, const DataType&, const DataSpace&)
			// only produces corrupted datasets in Debug mode (probably the HDF5 libraries would need to be build in Debug mode as well),
			// so we have to use the const char* version to better debug it.
			auto dataset = imageGroup.createDataSet(names[i].c_str(), type_id, dataspace);
			dataset.write(image.data(), type_id);

			writeAttribute(dataset, "CLASS", "IMAGE");
			writeAttribute(dataset, "IMAGE_VERSION", "1.2");
			writeAttribute(dataset, "IMAGE_SUBCLASS", "IMAGE_GRAYSCALE");

			writeAttribute(dataset, "dx", positions[i].x);
			writeAttribute(dataset, "dy", positions[i].y);

			dataset.close();
			++i;
		}
	} catch (H5::Exception& exception) {
		emit(s_scaleCalibrationStatus("Could not save the scale calibration", "Please select a writable working directory."));
	}
}

void ScaleCalibration::writePoint(H5::Group group, std::string name, POINT2 point) {
	using namespace H5;

	auto subgroup = H5::Group(group.createGroup(&name[0]));

	writeAttribute(subgroup, "x", point.x);
	writeAttribute(subgroup, "y", point.y);

	subgroup.close();
}

POINT2 ScaleCalibration::readPoint(H5::Group group, const std::string& name) {
	auto point = POINT2{};

	// Open point group
	try {
		auto pointGroup = group.openGroup(&name[0]);

		// Read x coordinate
		readAttribute(pointGroup, "x", &point.x);
		// Read y coordinate
		readAttribute(pointGroup, "y", &point.y);

	} catch (H5::GroupIException& exception) {
		throw;
	}

	return point;
}

void ScaleCalibration::writeAttribute(H5::H5Object& parent, std::string name, double value) {
	auto attr_dataspace = H5::DataSpace(H5S_SCALAR);
	// Create new string datatype for attribute
	auto strdatatype = H5::PredType::NATIVE_DOUBLE;
	auto attr = parent.createAttribute(name.c_str(), strdatatype, attr_dataspace);
	attr.write(strdatatype, &value);
	attr.close();
}

void ScaleCalibration::writeAttribute(H5::H5Object& parent, std::string name, std::string value) {
	auto attr_dataspace = H5::DataSpace(H5S_SCALAR);
	// Create new string datatype for attribute
	auto strdatatype = H5::StrType(H5::PredType::C_S1, value.size());
	auto attr = parent.createAttribute(name.c_str(), strdatatype, attr_dataspace);
	attr.write(strdatatype, value.c_str());
	attr.close();
}

void ScaleCalibration::readAttribute(const H5::H5Object& parent, std::string name, double* value) {
	auto attr = parent.openAttribute(name.c_str());
	auto type = attr.getDataType();
	// If this is not a double, return NaN
	if (type != H5::PredType::NATIVE_DOUBLE) {
		return;
	}
	attr.read(type, value);
	type.close();
	attr.close();
}

template <typename T>
void ScaleCalibration::__acquire() {
	setAcquisitionStatus(ACQUISITION_STATUS::STARTED);

	QMetaObject::invokeMethod(
		(*m_scanControl),
		[&m_scanControl = (*m_scanControl)]() { m_scanControl->stopAnnouncing(); },
		Qt::AutoConnection
	);
	// Set optical elements for brightfield/Brillouin imaging
	(*m_scanControl)->setPreset(ScanPreset::SCAN_BRIGHTFIELD);
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	// Get the current stage position
	m_startPosition = (*m_scanControl)->getPosition();

	/*
	 * We acquire three images here, one at the origin, and one each shifted in x- and y-direction.
	 */
	auto hysteresisCompensation{ 10.0 };// [µm] distance for compensation of the stage hysteresis
	// Construct the positions
	auto positions = std::vector<POINT2>{ { 0, 0 }, { m_Ds.x, 0 }, { 0, m_Ds.y } };

	// Acquire memory for image acquisition
	auto images = std::vector<std::vector<std::byte>>(positions.size());
	for (auto& image : images) {
		image.resize(m_cameraSettings.roi.bytesPerFrame);
	}

	(*m_camera)->startAcquisition(m_cameraSettings);

	auto iteration{ 0 };
	emit(s_scaleCalibrationAcquisitionProgress(iteration));
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
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		(*m_scanControl)->setPosition(positionAbsolute);
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

		/*
		 * Acquire the camera image
		 */

		 // acquire images
		(*m_camera)->getImageForAcquisition(&(images[iteration])[0], false);

		// Sometimes the uEye camera returns a black image (only zeros), we try to catch this here by
		// repeating the acquisition a maximum of 5 times
		// cast the vector type T
		auto images_ = (std::vector<T> *) &(images[iteration]);

		auto sum = simplemath::sum(*images_);
		auto tryCount{ 0 };
		while (sum == 0 && 5 > tryCount++) {
			(*m_camera)->getImageForAcquisition(&(images[iteration])[0], false);

			images_ = (std::vector<T> *) &(images[iteration]);
			sum = simplemath::sum(*images_);
		}

		++iteration;
		emit(s_scaleCalibrationAcquisitionProgress(iteration * 100.0 / images.size()));
	}

	// Stop the camera acquisition
	(*m_camera)->stopAcquisition();

	// Create input matrices for OpenCV from camera images
	auto imagesCV = std::vector<cv::Mat>(images.size());
	for (gsl::index i{ 0 }; i < images.size(); i++) {
		imagesCV[i] = cv::Mat(m_cameraSettings.roi.height_binned, m_cameraSettings.roi.width_binned, CV_8UC1, &(images[i])[0]);
	}

	/*
	 * Determine the shift in pixels
	 */

#ifdef _DEBUG
	 // Show the input images for debugging
	cv::imshow("Origin", imagesCV[0]);
	cv::imshow("Dx", imagesCV[1]);
	cv::imshow("Dy", imagesCV[2]);
#endif

	// We use template matching, so we have to create a template from the origin image
	auto padding = 200.0;
	auto size = imagesCV[0].size();
	auto templateROI = cv::Rect(padding, padding, size.width - 2 * padding, size.height - 2 * padding);
	auto templ = imagesCV[0](templateROI);

#ifdef _DEBUG
	cv::imshow("Template", templ);
#endif

	auto outputX = cv::Mat{};
	cv::matchTemplate(imagesCV[1], templ, outputX, cv::TemplateMatchModes::TM_SQDIFF);
	auto outputY = cv::Mat{};
	cv::matchTemplate(imagesCV[2], templ, outputY, cv::TemplateMatchModes::TM_SQDIFF);

	cv::normalize(outputX, outputX, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());
	cv::normalize(outputY, outputY, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());

	auto minValX = double{};
	auto maxValX = double{};
	auto minLocX = cv::Point{};
	auto maxLocX = cv::Point{};
	cv::minMaxLoc(outputX, &minValX, &maxValX, &minLocX, &maxLocX, cv::Mat());
	auto shiftDx = minLocX - cv::Point(padding, padding);

	auto minValY = double{};
	auto maxValY = double{};
	auto minLocY = cv::Point{};
	auto maxLocY = cv::Point{};
	cv::minMaxLoc(outputY, &minValY, &maxValY, &minLocY, &maxLocY, cv::Mat());
	auto shiftDy = minLocY - cv::Point(padding, padding);

	/*
	 * Construct the scale calibration
	 */
	m_scaleCalibration.micrometerToPixX = { -1 * shiftDx.x / m_Ds.x, shiftDx.y / m_Ds.x };
	m_scaleCalibration.micrometerToPixY = { -1 * shiftDy.x / m_Ds.y, shiftDy.y / m_Ds.y };
	try {
		// Can throw an exception (if the provided calibration is invalid):
		ScaleCalibrationHelper::initializeCalibrationFromMicrometer(&m_scaleCalibration);

		// Store the calibration in a file

		auto images_ = (std::vector<std::vector<T>> *) &images;
		save((*images_), positions);

		emit(s_scaleCalibrationChanged(m_scaleCalibration));

	} catch (std::exception& e) {
		emit(s_scaleCalibrationStatus("Scale calibration failed", "Please make sure there are distinct structures visible."));
	}

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

/*
 * Private slots
 */

void ScaleCalibration::acquire(std::unique_ptr <StorageWrapper>& storage) {}

void ScaleCalibration::acquire() {
	if (m_cameraSettings.readout.dataType == "unsigned short") {
		__acquire<unsigned short>();
	} else if (m_cameraSettings.readout.dataType == "unsigned char") {
		__acquire<unsigned char>();
	}
}

void ScaleCalibration::initialize() {
	// Get the current scale calibration from the scanControl
	m_scaleCalibration = (*m_scanControl)->getScaleCalibration();

	// Emit it to the main GUI thread
	emit(s_scaleCalibrationAcquisitionProgress(0.0));
	emit(s_scaleCalibrationChanged(m_scaleCalibration));
	emit(s_Ds_changed(m_Ds));
}

void ScaleCalibration::apply() {
	try {
		ScaleCalibrationHelper::initializeCalibrationFromMicrometer(&m_scaleCalibration);
		ScaleCalibrationHelper::initializeCalibrationFromPixel(&m_scaleCalibration);
		(*m_scanControl)->setScaleCalibration(m_scaleCalibration);
		emit(s_closeScaleCalibrationDialog());
	} catch (std::exception& e) {
		emit(s_scaleCalibrationStatus("Cannot apply scale calibration", "The provided scale calibration is invalid."));
	}
}

void ScaleCalibration::setTranslationDistanceX(double dx) {
	m_Ds.x = dx;
	emit(s_Ds_changed(m_Ds));
}

void ScaleCalibration::setTranslationDistanceY(double dy) {
	m_Ds.y = dy;
	emit(s_Ds_changed(m_Ds));
}

void ScaleCalibration::setMicrometerToPixX_x(double value) {
	m_scaleCalibration.micrometerToPixX.x = value;
	try {
		ScaleCalibrationHelper::initializeCalibrationFromMicrometer(&m_scaleCalibration);
		emit(s_scaleCalibrationChanged(m_scaleCalibration));
	} catch (std::exception& e) {
	}
}

void ScaleCalibration::setMicrometerToPixX_y(double value) {
	m_scaleCalibration.micrometerToPixX.y = value;
	try {
		ScaleCalibrationHelper::initializeCalibrationFromMicrometer(&m_scaleCalibration);
		emit(s_scaleCalibrationChanged(m_scaleCalibration));
	} catch (std::exception& e) {
	}
}

void ScaleCalibration::setMicrometerToPixY_x(double value) {
	m_scaleCalibration.micrometerToPixY.x = value;
	try {
		ScaleCalibrationHelper::initializeCalibrationFromMicrometer(&m_scaleCalibration);
		emit(s_scaleCalibrationChanged(m_scaleCalibration));
	} catch (std::exception& e) {
	}
}

void ScaleCalibration::setMicrometerToPixY_y(double value) {
	m_scaleCalibration.micrometerToPixY.y = value;
	try {
		ScaleCalibrationHelper::initializeCalibrationFromMicrometer(&m_scaleCalibration);
		emit(s_scaleCalibrationChanged(m_scaleCalibration));
	} catch (std::exception& e) {
	}
}

void ScaleCalibration::setPixToMicrometerX_x(double value) {
	m_scaleCalibration.pixToMicrometerX.x = value;
	try {
		ScaleCalibrationHelper::initializeCalibrationFromPixel(&m_scaleCalibration);
		emit(s_scaleCalibrationChanged(m_scaleCalibration));
	} catch (std::exception& e) {
	}
}

void ScaleCalibration::setPixToMicrometerX_y(double value) {
	m_scaleCalibration.pixToMicrometerX.y = value;
	try {
		ScaleCalibrationHelper::initializeCalibrationFromPixel(&m_scaleCalibration);
		emit(s_scaleCalibrationChanged(m_scaleCalibration));
	} catch (std::exception& e) {
	}
}

void ScaleCalibration::setPixToMicrometerY_x(double value) {
	m_scaleCalibration.pixToMicrometerY.x = value;
	try {
		ScaleCalibrationHelper::initializeCalibrationFromPixel(&m_scaleCalibration);
		emit(s_scaleCalibrationChanged(m_scaleCalibration));
	} catch (std::exception& e) {
	}
}

void ScaleCalibration::setPixToMicrometerY_y(double value) {
	m_scaleCalibration.pixToMicrometerY.y = value;
	try {
		ScaleCalibrationHelper::initializeCalibrationFromPixel(&m_scaleCalibration);
		emit(s_scaleCalibrationChanged(m_scaleCalibration));
	} catch (std::exception& e) {
	}
}
