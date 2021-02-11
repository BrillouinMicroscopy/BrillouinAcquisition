#include "stdafx.h"
#include "Brillouin.h"
#include "../../simplemath.h"
#include "../../logger.h"
#include "filesystem"

using namespace std::filesystem;

/*
 * Public definitions
 */

Brillouin::Brillouin(QObject* parent, Acquisition* acquisition, Camera** andor, ScanControl** scanControl)
	: AcquisitionMode(parent, acquisition, scanControl), m_andor(andor) {
	static QMetaObject::Connection connection = QWidget::connect(
		this,
		&Brillouin::s_scanOrderChanged,
		this,
		[this](SCAN_ORDER scanOrder) { updatePositions(); }
	);
	// Emit the initial positions
	updatePositions();
}

Brillouin::~Brillouin() {
	if (m_repetitionTimer) {
		m_repetitionTimer->stop();
		m_repetitionTimer->deleteLater();
	}
}

/*
 * Public slots
 */

void Brillouin::startRepetitions() {
	bool allowed = m_acquisition->enableMode(ACQUISITION_MODE::BRILLOUIN);
	if (!allowed) {
		return;
	}

	// If the repetition timer is running already, we stop the next repetition
	if (m_repetitionTimer != nullptr && m_repetitionTimer->isActive()) {
		m_repetitionTimer->stop();
		m_startOfLastRepetition.invalidate();
		finaliseRepetitions(m_currentRepetition, -2);
		setAcquisitionStatus(ACQUISITION_STATUS::STOPPED);
		return;
	}

	m_abort = false;

	auto info = std::string{ "Acquisition started." };
	qInfo(logInfo()) << info.c_str();

	m_currentRepetition = 0;
	m_startOfLastRepetition.start();

	m_repetitionTimer = new QTimer();
	QMetaObject::Connection connection = QWidget::connect(
		m_repetitionTimer,
		&QTimer::timeout,
		this,
		&Brillouin::waitForNextRepetition
	);
	m_repetitionTimer->start(100);
}

void Brillouin::waitForNextRepetition() {

	if (m_abort) {
		this->abortMode(m_acquisition->m_storage);
		return;
	}
	
	// Save the filename of the first repetition
	if (m_currentRepetition == 0) {
		m_baseFilename = m_acquisition->getCurrentFilename();
	}

	// Check if we have to start a new repetition or wait more
	auto timeSinceLast = int{ (int)(1e-3 * m_startOfLastRepetition.elapsed()) };
	if (m_currentRepetition == 0 || timeSinceLast >= m_settings.repetitions.interval * 60) {
		m_startOfLastRepetition.restart();
		m_repetitionTimer->stop();
		emit(s_totalProgress(m_currentRepetition, -1));

		if (m_settings.repetitions.filePerRepetition && m_currentRepetition != 0) {
			auto repetitionFilename = getRepetitionFilename();
			m_acquisition->openFile(repetitionFilename, true);
		}

		m_acquisition->newRepetition(ACQUISITION_MODE::BRILLOUIN);

		setAcquisitionStatus(ACQUISITION_STATUS::STARTED);
		acquire(m_acquisition->m_storage);

		if (m_abort) {
			this->abortMode(m_acquisition->m_storage);
			return;
		}
		m_currentRepetition++;
		// Check if this was the last repetition
		if (m_currentRepetition < m_settings.repetitions.count) {
			m_repetitionTimer->start(100);
			setAcquisitionStatus(ACQUISITION_STATUS::WAITFORREPETITION);
		} else {
			m_startOfLastRepetition.invalidate();
			// Cleanup after last repetition
			finaliseRepetitions();
			setAcquisitionStatus(ACQUISITION_STATUS::FINISHED);
		}
	} else {
		timeSinceLast = 1e-3 * m_startOfLastRepetition.elapsed();
		emit(s_totalProgress(m_currentRepetition, m_settings.repetitions.interval * 60 - timeSinceLast));
	}
}

void Brillouin::finaliseRepetitions() {
	finaliseRepetitions(m_settings.repetitions.count, -1);
}

void Brillouin::finaliseRepetitions(int nrFinishedRepetitions, int status) {
	emit(s_totalProgress(nrFinishedRepetitions, status));
	m_acquisition->disableMode(ACQUISITION_MODE::BRILLOUIN);
}

void Brillouin::setStepNumberX(int steps) {
	m_settings.xSteps = steps;
	determineScanOrder();
}

void Brillouin::setStepNumberY(int steps) {
	m_settings.ySteps = steps;
	determineScanOrder();
}

void Brillouin::setStepNumberZ(int steps) {
	m_settings.zSteps = steps;
	determineScanOrder();
}

void Brillouin::setXMin(double xMin) {
	m_settings.xMin = xMin;
	updatePositions();
}

void Brillouin::setXMax(double xMax) {
	m_settings.xMax = xMax;
	updatePositions();
}

void Brillouin::setYMin(double yMin) {
	m_settings.yMin = yMin;
	updatePositions();
}

void Brillouin::setYMax(double yMax) {
	m_settings.yMax = yMax;
	updatePositions();
}

void Brillouin::setZMin(double zMin) {
	m_settings.zMin = zMin;
	updatePositions();
}

void Brillouin::setZMax(double zMax) {
	m_settings.zMax = zMax;
	updatePositions();
}

void Brillouin::setSettings(const BRILLOUIN_SETTINGS& settings) {
	m_settings = settings;
}

/*
 *	Scan direction order related variables and functions
 */

void Brillouin::setScanOrderX(int x) {
	if (m_scanOrder.automatical) {
		emit(s_scanOrderChanged(m_scanOrder));
		return;
	}
	// switch values
	if (m_scanOrder.y == x) {
		m_scanOrder.y = m_scanOrder.x;
	}
	if (m_scanOrder.z == x) {
		m_scanOrder.z = m_scanOrder.x;
	}
	m_scanOrder.x = x;
	emit(s_scanOrderChanged(m_scanOrder));
}

void Brillouin::setScanOrderY(int y) {
	if (m_scanOrder.automatical) {
		emit(s_scanOrderChanged(m_scanOrder));
		return;
	}
	// switch values
	if (m_scanOrder.x == y) {
		m_scanOrder.x = m_scanOrder.y;
	}
	if (m_scanOrder.z == y) {
		m_scanOrder.z = m_scanOrder.y;
	}
	m_scanOrder.y = y;
	emit(s_scanOrderChanged(m_scanOrder));
}

void Brillouin::setScanOrderZ(int z) {
	if (m_scanOrder.automatical) {
		emit(s_scanOrderChanged(m_scanOrder));
		return;
	}
	// switch values
	if (m_scanOrder.x == z) {
		m_scanOrder.x = m_scanOrder.z;
	}
	if (m_scanOrder.y == z) {
		m_scanOrder.y = m_scanOrder.z;
	}
	m_scanOrder.z = z;
	emit(s_scanOrderChanged(m_scanOrder));
}

void Brillouin::setScanOrderAuto(bool automatical) {
	m_scanOrder.automatical = automatical;
	determineScanOrder();
}

void Brillouin::determineScanOrder() {
	if (m_scanOrder.automatical) {
		// determine scan order based on step numbers
		// highest step number first, then descending
		auto stepNumbers = std::vector<int>{ m_settings.xSteps, m_settings.ySteps, m_settings.zSteps };
		auto indices = simplemath::tag_sort_inverse(stepNumbers);
		auto order = std::vector<int>(stepNumbers.size());
		for (gsl::index jj{ 0 }; jj < order.size(); jj++) {
			order[indices[jj]] = jj;
		}

		m_scanOrder.x = order[0];
		m_scanOrder.y = order[1];
		m_scanOrder.z = order[2];

	}
	emit(s_scanOrderChanged(m_scanOrder));
}

std::vector<POINT3> Brillouin::getOrderedPositions() {
	return m_orderedPositionsRelative;
}

/*
 * Private definitions
 */

void Brillouin::abortMode(std::unique_ptr <StorageWrapper>& storage) {
	m_repetitionTimer->stop();
	m_startOfLastRepetition.invalidate();
	if (m_andor) {
		(*m_andor)->stopAcquisition();
	}

	if (m_scanControl) {
		(*m_scanControl)->setPreset(ScanPreset::SCAN_LASEROFF);
		(*m_scanControl)->setPosition(m_startPosition);
		QMetaObject::invokeMethod(
			(*m_scanControl),
			[&m_scanControl = (*m_scanControl)]() { m_scanControl->startAnnouncing(); },
			Qt::AutoConnection
		);
	}

	m_acquisition->disableMode(ACQUISITION_MODE::BRILLOUIN);

	// Here we wait until the storage object indicate it finished to write to the file.
	QEventLoop loop;
	auto connection = QWidget::connect(
		storage.get(),
		&StorageWrapper::finished,
		&loop,
		&QEventLoop::quit
	);
	QMetaObject::invokeMethod(
		storage.get(),
		[&storage = storage]() { storage.get()->s_finishedQueueing(); },
		Qt::AutoConnection
	);
	loop.exec();

	setAcquisitionStatus(ACQUISITION_STATUS::ABORTED);
	emit(s_positionChanged({ 0 , 0, 0 }, 0));
	emit(s_timeToCalibration(0));
}

void Brillouin::calibrate(std::unique_ptr <StorageWrapper>& storage) {
	// announce calibration start
	emit(s_calibrationRunning(true));

	// set exposure time for calibration
	if (m_andor) {
		(*m_andor)->setCalibrationExposureTime(m_settings.calibrationExposureTime);
	}

	// move optical elements to position for calibration
	if (m_scanControl) {
		(*m_scanControl)->setPreset(ScanPreset::SCAN_CALIBRATION);
	}
	Sleep(500);

	auto shift = 5.088; // this is the shift for water

	// acquire images
	auto rank_cal = 3;
	hsize_t dims_cal[3] = {
		(hsize_t)m_settings.nrCalibrationImages,
		(hsize_t)m_settings.camera.roi.height_binned,
		(hsize_t)m_settings.camera.roi.width_binned
	};

	auto images = std::vector<std::byte>((int64_t)m_settings.camera.roi.bytesPerFrame * m_settings.nrCalibrationImages);
	for (gsl::index mm{ 0 }; mm < m_settings.nrCalibrationImages; mm++) {
		if (m_abort) {
			this->abortMode(storage);
			return;
		}
		// acquire images
		auto pointerPos = (int64_t)m_settings.camera.roi.bytesPerFrame * mm;

		if (m_andor) {
			(*m_andor)->getImageForAcquisition(&images[pointerPos]);
		}
	}

	// the datetime has to be set here, otherwise it would be determined by the time the queue is processed
	auto date = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
		.toString(Qt::ISODateWithMs).toStdString();

	if (m_settings.camera.readout.dataType == "unsigned short") {
		// cast the image to unsigned short
		auto images_ = (std::vector<unsigned short> *) & images;
		auto cal = new CALIBRATION<unsigned short>(
			nrCalibrations,			// index
			*images_,				// data
			rank_cal,				// the rank of the calibration data
			dims_cal,				// the dimension of the calibration data
			m_settings.sample,		// the samplename
			shift,					// the Brillouin shift of the sample
			date,					// the datetime
			m_settings.calibrationExposureTime, // the exposure time of the calibration
			m_settings.camera.gain,
			m_settings.camera.roi
			);

		QMetaObject::invokeMethod(
			storage.get(),
			[&storage = storage, cal]() { storage.get()->s_enqueueCalibration(cal); },
			Qt::AutoConnection
		);
	} else if (m_settings.camera.readout.dataType == "unsigned char") {
		// cast the image to unsigned char
		auto images_ = (std::vector<unsigned char> *) & images;
		auto cal = new CALIBRATION<unsigned char>(
			nrCalibrations,			// index
			*images_,				// data
			rank_cal,				// the rank of the calibration data
			dims_cal,				// the dimension of the calibration data
			m_settings.sample,		// the samplename
			shift,					// the Brillouin shift of the sample
			date,					// the datetime
			m_settings.calibrationExposureTime, // the exposure time of the calibration
			m_settings.camera.gain,
			m_settings.camera.roi
			);

		QMetaObject::invokeMethod(
			storage.get(),
			[&storage = storage, cal]() { storage.get()->s_enqueueCalibration(cal); },
			Qt::AutoConnection
		);
	} else if (m_settings.camera.readout.dataType == "unsigned int") {
		// cast the image to unsigned char
		auto images_ = (std::vector<unsigned int> *) & images;
		auto cal = new CALIBRATION<unsigned int>(
			nrCalibrations,			// index
			*images_,				// data
			rank_cal,				// the rank of the calibration data
			dims_cal,				// the dimension of the calibration data
			m_settings.sample,		// the samplename
			shift,					// the Brillouin shift of the sample
			date,					// the datetime
			m_settings.calibrationExposureTime, // the exposure time of the calibration
			m_settings.camera.gain,
			m_settings.camera.roi
			);

		QMetaObject::invokeMethod(
			storage.get(),
			[&storage = storage, cal]() { storage.get()->s_enqueueCalibration(cal); },
			Qt::AutoConnection
		);
	}

	nrCalibrations++;

	// revert optical elements to position for brightfield/Brillouin imaging
	if (m_scanControl) {
		(*m_scanControl)->setPreset(ScanPreset::SCAN_BRILLOUIN);
	}

	// reset exposure time
	if (m_andor) {
		(*m_andor)->setCalibrationExposureTime(m_settings.camera.exposureTime);
	}
	Sleep(500);
}

/*
 * Construct positions vector with correct order of scan directions
 */
void Brillouin::updatePositions() {
	auto nrPositions = m_settings.xSteps * m_settings.ySteps * m_settings.zSteps;

	// Adjust positions vector
	m_orderedPositions.resize(nrPositions);
	m_orderedPositionsRelative.resize(nrPositions);
	m_orderedIndices.resize(nrPositions);
	// vector indicating if a new line just started and a calibration is allowed
	m_calibrationAllowed.resize(nrPositions);

	// construct directions vector
	std::vector<std::vector<double>> directions(3);
	directions[m_scanOrder.x] = simplemath::linspace(m_settings.xMin, m_settings.xMax, m_settings.xSteps);
	directions[m_scanOrder.y] = simplemath::linspace(m_settings.yMin, m_settings.yMax, m_settings.ySteps);
	directions[m_scanOrder.z] = simplemath::linspace(m_settings.zMin, m_settings.zMax, m_settings.zSteps);

	int ll{ 0 };
	std::vector<double> position(3);
	std::vector<int> indices(3);
	for (gsl::index ii{ 0 }; ii < directions[2].size(); ii++) {
		for (gsl::index jj{ 0 }; jj < directions[1].size(); jj++) {
			for (gsl::index kk{ 0 }; kk < directions[0].size(); kk++) {

				// construct indices vector
				indices[0] = kk;
				indices[1] = jj;
				indices[2] = ii;

				// construct position vector
				position[0] = directions[0][kk];
				position[1] = directions[1][jj];
				position[2] = directions[2][ii];

				// calculate stage positions
				m_orderedPositionsRelative[ll] = POINT3{ position[m_scanOrder.x], position[m_scanOrder.y], position[m_scanOrder.z] };
				m_orderedPositions[ll] = m_orderedPositionsRelative[ll] + m_startPosition;

				// fill index vectors
				m_orderedIndices[ll] = INDEX3{ indices[m_scanOrder.x], indices[m_scanOrder.y], indices[m_scanOrder.z] };

				// set vector element to true if a new line started
				if (kk == 0) {
					m_calibrationAllowed[ll] = true;
				} else {
					m_calibrationAllowed[ll] = false;
				}
				ll++;
			}
		}
	}
	emit(s_orderedPositionsChanged(m_orderedPositionsRelative));
}

std::string Brillouin::getRepetitionFilename() {
	auto rawFilename = m_baseFilename.substr(0, m_baseFilename.find_last_of("."));
	auto fileEnding = m_baseFilename.substr(m_baseFilename.find_last_of("."), std::string::npos);

	// Get the number of digits necessary for the desired repetition count
	auto nrDigits = (int)floor(log10(m_settings.repetitions.count) + 1);

	auto formatString = std::string{ rawFilename + "_rep%0" + std::to_string(nrDigits) + "d" + fileEnding};

	auto string = QString{};
	string.sprintf(formatString.c_str(), m_currentRepetition);

	return string.toStdString();
}


/*
 * Private slots
 */

void Brillouin::acquire(std::unique_ptr <StorageWrapper>& storage) {
	setAcquisitionStatus(ACQUISITION_STATUS::STARTED);
	// prepare camera for image acquisition

	if (m_andor) {
		(*m_andor)->startAcquisition(m_settings.camera);
		m_settings.camera = (*m_andor)->getSettings();
	} else {
		m_abort = true;
		return;
	}

	if (m_scanControl) {
		QMetaObject::invokeMethod(
			(*m_scanControl),
			[&m_scanControl = (*m_scanControl)]() { m_scanControl->stopAnnouncing(); },
			Qt::AutoConnection
		);
		// set optical elements for brightfield/Brillouin imaging
		(*m_scanControl)->setPreset(ScanPreset::SCAN_BRILLOUIN);
	} else {
		m_abort = true;
		return;
	}
	Sleep(500);

	// get current stage position
	if (m_scanControl) {
		m_startPosition = (*m_scanControl)->getPosition();
		// Enable measurement mode (so the AOI display is correct).
		(*m_scanControl)->enableMeasurementMode(true);
	} else {
		m_abort = true;
		return;
	}

	auto commentIn = std::string{ "Brillouin data" };
	storage->setComment(commentIn);

	storage->setResolution("x", m_settings.xSteps);
	storage->setResolution("y", m_settings.ySteps);
	storage->setResolution("z", m_settings.zSteps);

	auto resolutionXout = storage->getResolution("x");

	writeScaleCalibration(storage, ACQUISITION_MODE::BRILLOUIN);

	/*
	 * Update the positions vector
	 */
	updatePositions();

	/*
	 * Construct positions vector for H5 file with row-major order: z, x, y
	 */
	 // construct directions vectors
	auto directionsX{ simplemath::linspace(m_settings.xMin, m_settings.xMax, m_settings.xSteps) };
	auto directionsY{ simplemath::linspace(m_settings.yMin, m_settings.yMax, m_settings.ySteps) };
	auto directionsZ{ simplemath::linspace(m_settings.zMin, m_settings.zMax, m_settings.zSteps) };

	// total number of positions to measure
	auto nrPositions = m_settings.xSteps * m_settings.ySteps * m_settings.zSteps;
	auto positionsX = std::vector<double>(nrPositions);
	auto positionsY = std::vector<double>(nrPositions);
	auto positionsZ = std::vector<double>(nrPositions);
	auto posIndex{ 0 };
	for (gsl::index ii{ 0 }; ii < m_settings.zSteps; ii++) {
		for (gsl::index jj{ 0 }; jj < m_settings.xSteps; jj++) {
			for (gsl::index kk{ 0 }; kk < m_settings.ySteps; kk++) {
				positionsX[posIndex] = directionsX[jj] + m_startPosition.x;
				positionsY[posIndex] = directionsY[kk] + m_startPosition.y;
				positionsZ[posIndex] = directionsZ[ii] + m_startPosition.z;
				posIndex++;
			}
		}
	}

	auto rank{ 3 };
	auto dims = new hsize_t[rank];
	dims[0] = m_settings.zSteps;
	dims[1] = m_settings.xSteps;
	dims[2] = m_settings.ySteps;

	storage->setPositions("x", positionsX, rank, dims);
	storage->setPositions("y", positionsY, rank, dims);
	storage->setPositions("z", positionsZ, rank, dims);
	delete[] dims;

	// do actual measurement
	QMetaObject::invokeMethod(
		storage.get(),
		[&storage = storage]() { storage.get()->startWritingQueues(); },
		Qt::AutoConnection
	);

	auto rank_data{ 3 };
	hsize_t dims_data[3] = {
		(hsize_t)m_settings.camera.frameCount,
		(hsize_t)m_settings.camera.roi.height_binned,
		(hsize_t)m_settings.camera.roi.width_binned
	};

	// reset number of calibrations
	nrCalibrations = 1;
	// do pre calibration
	if (m_settings.preCalibration) {
		calibrate(storage);
	}

	auto measurementTimer = QElapsedTimer{};
	measurementTimer.start();

	auto calibrationTimer = QElapsedTimer{};
	calibrationTimer.start();

	// move stage to first position, wait 50 ms for it to finish
	if (m_scanControl) {
		(*m_scanControl)->setPosition(m_orderedPositions[0]);
	} else {
		m_abort = true;
		return;
	}
	Sleep(50);

	for (gsl::index ll{ 0 }; ll < nrPositions; ll++) {

		// do live calibration if required and possible at the moment
		if (m_settings.conCalibration && m_calibrationAllowed[ll]) {
			if (calibrationTimer.elapsed() > (60e3 * m_settings.conCalibrationInterval)) {
				calibrate(storage);
				calibrationTimer.start();
				// After we calibrated, we move back to the current position
				if (m_scanControl) {
					(*m_scanControl)->setPosition(m_orderedPositions[ll]);
				} else {
					m_abort = true;
					return;
				}
				Sleep(100);
			}
		}

		auto nextCalibration = int{ (int)(100 * (1e-3 * calibrationTimer.elapsed()) / (60 * m_settings.conCalibrationInterval)) };
		emit(s_timeToCalibration(nextCalibration));

		std::vector<std::byte> images(m_settings.camera.roi.bytesPerFrame * m_settings.camera.frameCount);

		for (gsl::index mm{ 0 }; mm < m_settings.camera.frameCount; mm++) {
			if (m_abort) {
				m_abort = true;
				return;
			}
			emit(s_positionChanged(m_orderedPositions[ll] - m_startPosition, mm + 1));
			// acquire images
			auto pointerPos = (int64_t)m_settings.camera.roi.bytesPerFrame * mm;

			if (m_andor) {
				(*m_andor)->getImageForAcquisition(&images[pointerPos]);
			} else {
				m_abort = true;
				return;
			}
		}


		// asynchronously write image to disk
		// the datetime has to be set here, otherwise it would be determined by the time the queue is processed
		auto date = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
			.toString(Qt::ISODateWithMs).toStdString();

		if (m_settings.camera.readout.dataType == "unsigned short") {
			// cast the image to unsigned short
			auto images_ = (std::vector<unsigned short> *) & images;
			auto img = new IMAGE<unsigned short>(
				m_orderedIndices[ll].x,
				m_orderedIndices[ll].y,
				m_orderedIndices[ll].z,
				rank_data,
				dims_data,
				date,
				*images_,
				m_settings.camera.exposureTime,
				m_settings.camera.gain,
				m_settings.camera.roi
			);

			QMetaObject::invokeMethod(
				storage.get(),
				[&storage = storage, img]() { storage.get()->s_enqueuePayload(img); },
				Qt::AutoConnection
			);
		} else if (m_settings.camera.readout.dataType == "unsigned char") {
			// cast the image to unsigned char
			auto images_ = (std::vector<unsigned char> *) & images;
			auto img = new IMAGE<unsigned char>(
				m_orderedIndices[ll].x,
				m_orderedIndices[ll].y,
				m_orderedIndices[ll].z,
				rank_data,
				dims_data,
				date,
				*images_,
				m_settings.camera.exposureTime,
				m_settings.camera.gain,
				m_settings.camera.roi
			);

			QMetaObject::invokeMethod(
				storage.get(),
				[&storage = storage, img]() { storage.get()->s_enqueuePayload(img); },
				Qt::AutoConnection
			);
		} else if (m_settings.camera.readout.dataType == "unsigned int") {
			// cast the image to unsigned char
			auto images_ = (std::vector<unsigned int> *) & images;
			auto img = new IMAGE<unsigned int>(
				m_orderedIndices[ll].x,
				m_orderedIndices[ll].y,
				m_orderedIndices[ll].z,
				rank_data,
				dims_data,
				date,
				*images_,
				m_settings.camera.exposureTime,
				m_settings.camera.gain,
				m_settings.camera.roi
			);

			QMetaObject::invokeMethod(
				storage.get(),
				[&storage = storage, img]() { storage.get()->s_enqueuePayload(img); },
				Qt::AutoConnection
			);
		}

		// move stage to next position
		if (ll < ((gsl::index)nrPositions - 1)) {
			if (m_scanControl) {
				(*m_scanControl)->setPosition(m_orderedPositions[ll + 1]);
			} else {
				m_abort = true;
				return;
			}
		}

		auto percentage{ 100 * (double)(ll + 1) / nrPositions };
		auto remaining{ (int)(1e-3 * measurementTimer.elapsed() / (ll + 1) * ((int64_t)nrPositions - ll + 1)) };
		emit(s_repetitionProgress(percentage, remaining));
	}
	// do post calibration
	if (m_settings.postCalibration) {
		calibrate(storage);
	}

	// close camera libraries, clear buffers
	if (m_andor) {
		(*m_andor)->stopAcquisition();
	} else {
		m_abort = true;
		return;
	}

	if (m_scanControl) {
		(*m_scanControl)->setPreset(ScanPreset::SCAN_LASEROFF);

		(*m_scanControl)->setPosition(m_startPosition);
		(*m_scanControl)->enableMeasurementMode(false);
		emit(s_positionChanged({ 0, 0, 0 }, 0));
		QMetaObject::invokeMethod(
			(*m_scanControl),
			[&m_scanControl = (*m_scanControl)]() { m_scanControl->startAnnouncing(); },
			Qt::AutoConnection
		);
	} else {
		m_abort = true;
		return;
	}

	// Here we wait until the storage object indicate it finished to write to the file.
	QEventLoop loop;
	auto connection = QWidget::connect(
		storage.get(),
		&StorageWrapper::finished,
		&loop,
		&QEventLoop::quit
	);
	QMetaObject::invokeMethod(
		storage.get(),
		[&storage = storage]() { storage.get()->s_finishedQueueing(); },
		Qt::AutoConnection
	);
	loop.exec();

	auto info = std::string{ "Acquisition finished." };
	qInfo(logInfo()) << info.c_str();
	emit(s_calibrationRunning(false));
	setAcquisitionStatus(ACQUISITION_STATUS::FINISHED);
	emit(s_timeToCalibration(0));
}