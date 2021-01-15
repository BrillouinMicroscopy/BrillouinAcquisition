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

	auto parentObject{ parent() };
	if (parentObject->isWidgetType()) {
		m_Dialog = new QDialog((QWidget*)parent(), Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
		m_Dialog->setWindowTitle("Acquire scale calibration");
		m_Dialog->setWindowModality(Qt::ApplicationModal);

		m_ui.setupUi(m_Dialog);

		m_scaleCalibration = (*m_scanControl)->getScaleCalibration();

		// Initialize all values
		m_ui.dx->setValue(m_Dx);
		m_ui.dy->setValue(m_Dy);

		updateScaleCalibrationBoxes();

		// Connect close signal
		auto connection = QWidget::connect(
			m_Dialog,
			&QDialog::rejected,
			this,
			[this]() { closeDialog(); }
		);

		// Connect push buttons
		connection = QWidget::connect(
			m_ui.button_cancel,
			&QPushButton::clicked,
			this,
			[this]() { on_buttonCancel_clicked(); }
		);
		connection = QWidget::connect(
			m_ui.button_apply,
			&QPushButton::clicked,
			this,
			[this]() { on_buttonApply_clicked(); }
		);
		connection = QWidget::connect(
			m_ui.button_acquire,
			&QPushButton::clicked,
			this,
			[this]() { on_buttonAcquire_clicked(); }
		);

		// Connect translation distance boxes
		connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
			m_ui.dx,
			&QDoubleSpinBox::valueChanged,
			this,
			[this](double dx) { setTranslationDistanceX(dx); }
		);
		connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
			m_ui.dy,
			&QDoubleSpinBox::valueChanged,
			this,
			[this](double dy) { setTranslationDistanceY(dy); }
		);

		// Connect scale calibration boxes
		connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
			m_ui.micrometerToPixX_x,
			&QDoubleSpinBox::valueChanged,
			this,
			[this](double value) { setMicrometerToPixX_x(value); }
		);
		connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
			m_ui.micrometerToPixX_y,
			&QDoubleSpinBox::valueChanged,
			this,
			[this](double value) { setMicrometerToPixX_y(value); }
		);
		connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
			m_ui.micrometerToPixY_x,
			&QDoubleSpinBox::valueChanged,
			this,
			[this](double value) { setMicrometerToPixY_x(value); }
		);
		connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
			m_ui.micrometerToPixY_y,
			&QDoubleSpinBox::valueChanged,
			this,
			[this](double value) { setMicrometerToPixY_y(value); }
		);

		connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
			m_ui.pixToMicrometerX_x,
			&QDoubleSpinBox::valueChanged,
			this,
			[this](double value) { setPixToMicrometerX_x(value); }
		);
		connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
			m_ui.pixToMicrometerX_y,
			&QDoubleSpinBox::valueChanged,
			this,
			[this](double value) { setPixToMicrometerX_y(value); }
		);
		connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
			m_ui.pixToMicrometerY_x,
			&QDoubleSpinBox::valueChanged,
			this,
			[this](double value) { setPixToMicrometerY_x(value); }
		);
		connection = QWidget::connect<void(QDoubleSpinBox::*)(double)>(
			m_ui.pixToMicrometerY_y,
			&QDoubleSpinBox::valueChanged,
			this,
			[this](double value) { setPixToMicrometerY_y(value); }
		);

		

		m_Dialog->show();

		// reset abort flag
		m_abort = false;
	}
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

void ScaleCalibration::save(std::vector<std::vector<unsigned char>> images, CAMERA_SETTINGS settings, std::vector<POINT2> positions) {
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

	// Write images and set positions as attributes
	auto imageGroup = root.createGroup("images");
	hsize_t dims[3] = {
		(hsize_t)settings.frameCount,
		(hsize_t)settings.roi.height_binned,
		(hsize_t)settings.roi.width_binned
	};
	auto names = std::vector<std::string>{ "origin", "dx", "dy" };
	auto i = gsl::index{ 0 };
	for (const auto& image : images) {
		// Check that we don't run into trouble iterating over two arrays
		if (i >= names.size()) {
			continue;
		}
		auto dataspace = H5::DataSpace(3, dims, dims);
		// For some unknown reason using the overloaded function createDataSet(const H5std_string&, const DataType&, const DataSpace&)
		// only produces corrupted datasets in Debug mode (probably the HDF5 libraries would need to be build in Debug mode as well),
		// so we have to use the const char* version to better debug it.
		// TODO: Correctly determine the correct DataType from the image (NATIVE_CHAR might fail for other cameras).
		auto dataset = imageGroup.createDataSet(names[i].c_str(), H5::PredType::NATIVE_CHAR, dataspace);
		dataset.write(image.data(), H5::PredType::NATIVE_CHAR);

		writeAttribute(dataset, "CLASS", "IMAGE");
		writeAttribute(dataset, "IMAGE_VERSION", "1.2");
		writeAttribute(dataset, "IMAGE_SUBCLASS", "IMAGE_GRAYSCALE");

		writeAttribute(dataset, "dx", positions[i].x);
		writeAttribute(dataset, "dy", positions[i].y);

		dataset.close();
		++i;
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

	} catch (H5::GroupIException exception) {
		// Exception
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

void ScaleCalibration::readAttribute(H5::H5Object& parent, std::string name, double* value) {
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
	auto positions = std::vector<POINT2>{ { 0, 0 }, { m_Dx, 0 }, { 0, m_Dy } };

	// Acquire memory for image acquisition
	auto images = std::vector<std::vector<unsigned char>>(positions.size());
	for (auto& image : images) {
		image.resize(bytesPerFrame);
	}

	(*m_camera)->startAcquisition(cameraSettings);

	auto iteration{ 0 };
	m_ui.progress->setValue(iteration);
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
		m_ui.progress->setValue(iteration * 100.0 / images.size());
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
	m_scaleCalibration.micrometerToPixX = { -1 * shiftDx.x / m_Dx, shiftDx.y / m_Dx };
	m_scaleCalibration.micrometerToPixY = { -1 * shiftDy.x / m_Dy, shiftDy.y / m_Dy };
	ScaleCalibrationHelper::initializeCalibrationFromMicrometer(&m_scaleCalibration);

	// Store the calibration in a file
	save(images, cameraSettings, positions);

	updateScaleCalibrationBoxes();

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

void ScaleCalibration::on_buttonCancel_clicked() {
	closeDialog();
}

void ScaleCalibration::on_buttonApply_clicked() {
	// Apply the scale calibration
	// TODO: Check if it's valid before applying
	(*m_scanControl)->setScaleCalibration(m_scaleCalibration);
	closeDialog();
}

void ScaleCalibration::on_buttonAcquire_clicked() {
	acquire();
}

void ScaleCalibration::closeDialog() {
	// Disable the scale calibration mode
	m_acquisition->disableMode(ACQUISITION_MODE::SCALECALIBRATION);
	if (m_Dialog != nullptr) {
		// Hide the scale calibration dialog
		m_Dialog->hide();
	}
}

void ScaleCalibration::setTranslationDistanceX(double dx) {
	m_Dx = dx;
}

void ScaleCalibration::setTranslationDistanceY(double dy) {
	m_Dy = dy;
}

void ScaleCalibration::updateScaleCalibrationBoxes() {
	// We have to block the signals so that programmatically setting new values
	// doesn't trigger a new round of calculations
	m_ui.micrometerToPixX_x->blockSignals(true);
	m_ui.micrometerToPixX_y->blockSignals(true);
	m_ui.micrometerToPixY_x->blockSignals(true);
	m_ui.micrometerToPixY_y->blockSignals(true);

	m_ui.pixToMicrometerX_x->blockSignals(true);
	m_ui.pixToMicrometerX_y->blockSignals(true);
	m_ui.pixToMicrometerY_x->blockSignals(true);
	m_ui.pixToMicrometerY_y->blockSignals(true);

	m_ui.micrometerToPixX_x->setValue(m_scaleCalibration.micrometerToPixX.x);
	m_ui.micrometerToPixX_y->setValue(m_scaleCalibration.micrometerToPixX.y);
	m_ui.micrometerToPixY_x->setValue(m_scaleCalibration.micrometerToPixY.x);
	m_ui.micrometerToPixY_y->setValue(m_scaleCalibration.micrometerToPixY.y);

	m_ui.pixToMicrometerX_x->setValue(m_scaleCalibration.pixToMicrometerX.x);
	m_ui.pixToMicrometerX_y->setValue(m_scaleCalibration.pixToMicrometerX.y);
	m_ui.pixToMicrometerY_x->setValue(m_scaleCalibration.pixToMicrometerY.x);
	m_ui.pixToMicrometerY_y->setValue(m_scaleCalibration.pixToMicrometerY.y);

	m_ui.micrometerToPixX_x->blockSignals(false);
	m_ui.micrometerToPixX_y->blockSignals(false);
	m_ui.micrometerToPixY_x->blockSignals(false);
	m_ui.micrometerToPixY_y->blockSignals(false);

	m_ui.pixToMicrometerX_x->blockSignals(false);
	m_ui.pixToMicrometerX_y->blockSignals(false);
	m_ui.pixToMicrometerY_x->blockSignals(false);
	m_ui.pixToMicrometerY_y->blockSignals(false);
}

void ScaleCalibration::setMicrometerToPixX_x(double value) {
	m_scaleCalibration.micrometerToPixX.x = value;
	ScaleCalibrationHelper::initializeCalibrationFromMicrometer(&m_scaleCalibration);
	updateScaleCalibrationBoxes();
}

void ScaleCalibration::setMicrometerToPixX_y(double value) {
	m_scaleCalibration.micrometerToPixX.y = value;
	ScaleCalibrationHelper::initializeCalibrationFromMicrometer(&m_scaleCalibration);
	updateScaleCalibrationBoxes();
}

void ScaleCalibration::setMicrometerToPixY_x(double value) {
	m_scaleCalibration.micrometerToPixY.x = value;
	ScaleCalibrationHelper::initializeCalibrationFromMicrometer(&m_scaleCalibration);
	updateScaleCalibrationBoxes();
}

void ScaleCalibration::setMicrometerToPixY_y(double value) {
	m_scaleCalibration.micrometerToPixY.y = value;
	ScaleCalibrationHelper::initializeCalibrationFromMicrometer(&m_scaleCalibration);
	updateScaleCalibrationBoxes();
}

void ScaleCalibration::setPixToMicrometerX_x(double value) {
	m_scaleCalibration.pixToMicrometerX.x = value;
	ScaleCalibrationHelper::initializeCalibrationFromPixel(&m_scaleCalibration);
	updateScaleCalibrationBoxes();
}

void ScaleCalibration::setPixToMicrometerX_y(double value) {
	m_scaleCalibration.pixToMicrometerX.y = value;
	ScaleCalibrationHelper::initializeCalibrationFromPixel(&m_scaleCalibration);
	updateScaleCalibrationBoxes();
}

void ScaleCalibration::setPixToMicrometerY_x(double value) {
	m_scaleCalibration.pixToMicrometerY.x = value;
	ScaleCalibrationHelper::initializeCalibrationFromPixel(&m_scaleCalibration);
	updateScaleCalibrationBoxes();
}

void ScaleCalibration::setPixToMicrometerY_y(double value) {
	m_scaleCalibration.pixToMicrometerY.y = value;
	ScaleCalibrationHelper::initializeCalibrationFromPixel(&m_scaleCalibration);
	updateScaleCalibrationBoxes();
}
