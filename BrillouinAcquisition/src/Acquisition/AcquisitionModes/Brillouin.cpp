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
	: AcquisitionMode(parent, acquisition), m_andor(andor), m_scanControl(scanControl) {
}

Brillouin::~Brillouin() {}

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

	std::string info = "Acquisition started.";
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

	// Check if we have to start a new repetition or wait more
	int timeSinceLast{ (int)(1e-3 * m_startOfLastRepetition.elapsed()) };
	if (m_currentRepetition == 0 || timeSinceLast >= m_settings.repetitions.interval * 60) {
		m_startOfLastRepetition.restart();
		m_repetitionTimer->stop();
		emit(s_totalProgress(m_currentRepetition, -1));
		m_acquisition->newRepetition(ACQUISITION_MODE::BRILLOUIN);

		setAcquisitionStatus(ACQUISITION_STATUS::STARTED);
		acquire(m_acquisition->m_storage);
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

void Brillouin::setSettings(BRILLOUIN_SETTINGS settings) {
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

int Brillouin::getScanOrderX() { return m_scanOrder.x; }
int Brillouin::getScanOrderY() { return m_scanOrder.y; }
int Brillouin::getScanOrderZ() { return m_scanOrder.z; }

void Brillouin::setScanOrderAuto(bool automatical) {
	m_scanOrder.automatical = automatical;
	determineScanOrder();
}

void Brillouin::determineScanOrder() {
	if (m_scanOrder.automatical) {
		// determine scan order based on step numbers
		// highest step number first, then descending
		std::vector<int> stepNumbers{ m_settings.xSteps, m_settings.ySteps, m_settings.zSteps };
		auto indices = simplemath::tag_sort_inverse(stepNumbers);
		std::vector<int> order(stepNumbers.size());
		for (int jj{ 0 }; jj < order.size(); jj++) {
			order[indices[jj]] = jj;
		}

		m_scanOrder.x = order[0];
		m_scanOrder.y = order[1];
		m_scanOrder.z = order[2];
	}
	emit(s_scanOrderChanged(m_scanOrder));
}

void Brillouin::getScanOrder() {
	determineScanOrder();
}

/*
 * Private definitions
 */

void Brillouin::abortMode(std::unique_ptr <StorageWrapper>& storage) {
	m_repetitionTimer->stop();
	m_startOfLastRepetition.invalidate();

	(*m_andor)->stopAcquisition();

	(*m_scanControl)->setPreset(ScanPreset::SCAN_LASEROFF);

	(*m_scanControl)->setPosition(m_startPosition);

	QMetaObject::invokeMethod(
		(*m_scanControl),
		[&m_scanControl = (*m_scanControl)]() { m_scanControl->startAnnouncing(); },
		Qt::AutoConnection
	);

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
	(*m_andor)->setCalibrationExposureTime(m_settings.calibrationExposureTime);

	// move optical elements to position for calibration
	(*m_scanControl)->setPreset(ScanPreset::SCAN_CALIBRATION);
	Sleep(500);

	double shift = 5.088; // this is the shift for water

	// acquire images
	int rank_cal = 3;
	hsize_t dims_cal[3] = { (hsize_t)m_settings.nrCalibrationImages, (hsize_t)m_settings.camera.roi.height, (hsize_t)m_settings.camera.roi.width };

	int bytesPerFrame = 2 * m_settings.camera.roi.width * m_settings.camera.roi.height;
	std::vector<unsigned char> images((int64_t)bytesPerFrame * m_settings.nrCalibrationImages);
	for (gsl::index mm = 0; mm < m_settings.nrCalibrationImages; mm++) {
		if (m_abort) {
			this->abortMode(storage);
			return;
		}
		// acquire images
		int64_t pointerPos = (int64_t)bytesPerFrame * mm;
		(*m_andor)->getImageForAcquisition(&images[pointerPos]);
	}
	// cast the vector to unsigned short
	std::vector<unsigned short>* images_ = (std::vector<unsigned short>*) & images;

	std::string binning = getBinningString();

	// the datetime has to be set here, otherwise it would be determined by the time the queue is processed
	std::string date = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
		.toString(Qt::ISODateWithMs).toStdString();
	CALIBRATION* cal = new CALIBRATION(
		nrCalibrations,			// index
		*images_,				// data
		rank_cal,				// the rank of the calibration data
		dims_cal,				// the dimension of the calibration data
		m_settings.sample,		// the samplename
		shift,					// the Brillouin shift of the sample
		date,					// the datetime
		m_settings.calibrationExposureTime, // the exposure time of the calibration
		m_settings.camera.gain,
		binning
	);

	QMetaObject::invokeMethod(
		storage.get(),
		[&storage = storage, cal]() { storage.get()->s_enqueueCalibration(cal); },
		Qt::AutoConnection
	);

	nrCalibrations++;

	// revert optical elements to position for brightfield/Brillouin imaging
	(*m_scanControl)->setPreset(ScanPreset::SCAN_BRILLOUIN);

	// reset exposure time
	(*m_andor)->setCalibrationExposureTime(m_settings.camera.exposureTime);
	Sleep(500);
}

std::string Brillouin::getBinningString() {
	std::string binning{ "1x1" };
	if (m_settings.camera.roi.binning == L"8x8") {
		binning = "1x1";
	} else if (m_settings.camera.roi.binning == L"4x4") {
		binning = "1x1";
	} else if (m_settings.camera.roi.binning == L"2x2") {
		binning = "1x1";
	}
	return binning;
}

/*
 * Private slots
 */

void Brillouin::acquire(std::unique_ptr <StorageWrapper>& storage) {
	setAcquisitionStatus(ACQUISITION_STATUS::STARTED);
	// prepare camera for image acquisition
	(*m_andor)->startAcquisition(m_settings.camera);
	m_settings.camera = (*m_andor)->getSettings();
	QMetaObject::invokeMethod(
		(*m_scanControl),
		[&m_scanControl = (*m_scanControl)]() { m_scanControl->stopAnnouncing(); },
		Qt::AutoConnection
	);
	// set optical elements for brightfield/Brillouin imaging
	(*m_scanControl)->setPreset(ScanPreset::SCAN_BRILLOUIN);
	Sleep(500);

	// get current stage position
	m_startPosition = (*m_scanControl)->getPosition();

	std::string commentIn = "Brillouin data";
	storage->setComment(commentIn);

	storage->setResolution("x", m_settings.xSteps);
	storage->setResolution("y", m_settings.ySteps);
	storage->setResolution("z", m_settings.zSteps);

	int resolutionXout = storage->getResolution("x");

	// total number of positions to measure
	int nrPositions = m_settings.xSteps * m_settings.ySteps * m_settings.zSteps;

	/*
	 *	Construct positions vector with correct order of scan directions
	 */

	// Create positions vector
	std::vector<POINT3> orderedPositions(nrPositions);
	std::vector<int> indexX(nrPositions, 0);
	std::vector<int> indexY(nrPositions, 0);
	std::vector<int> indexZ(nrPositions, 0);
	// vector indicating if a new line just started and a calibration is allowed
	std::vector<bool> calibrationAllowed(nrPositions, false);

	int indX = getScanOrderX();
	int indY = getScanOrderY();
	int indZ = getScanOrderZ();

	// construct directions vector
	std::vector<std::vector<double>> directions(3);
	directions[indX] = simplemath::linspace(m_settings.xMin, m_settings.xMax, m_settings.xSteps);
	directions[indY] = simplemath::linspace(m_settings.yMin, m_settings.yMax, m_settings.ySteps);
	directions[indZ] = simplemath::linspace(m_settings.zMin, m_settings.zMax, m_settings.zSteps);

	int ll{ 0 };
	std::vector<double> position(3);
	std::vector<int> indices(3);
	for (gsl::index ii = 0; ii < directions[2].size(); ii++) {
		for (gsl::index jj = 0; jj < directions[1].size(); jj++) {
			for (gsl::index kk = 0; kk < directions[0].size(); kk++) {

				// construct indices vector
				indices[0] = kk;
				indices[1] = jj;
				indices[2] = ii;
				
				// construct position vector
				position[0] = directions[0][kk];
				position[1] = directions[1][jj];
				position[2] = directions[2][ii];

				// calculate stage positions
				orderedPositions[ll] = POINT3{ position[indX], position[indY], position[indZ] } + m_startPosition;

				// fill index vectors
				indexX[ll] = indices[indX];
				indexY[ll] = indices[indY];
				indexZ[ll] = indices[indZ];

				// set vector element to true if a new line started
				if (kk == 0) {
					calibrationAllowed[ll] = true;
				}
				ll++;
			}
		}
	}

	/*
	 *	Construct positions vector for H5 file with row-major order: z, x, y
	 */

	std::vector<double> positionsX(nrPositions);
	std::vector<double> positionsY(nrPositions);
	std::vector<double> positionsZ(nrPositions);
	ll = 0;
	for (gsl::index ii = 0; ii < m_settings.zSteps; ii++) {
		for (gsl::index jj = 0; jj < m_settings.xSteps; jj++) {
			for (gsl::index kk = 0; kk < m_settings.ySteps; kk++) {
				positionsX[ll] = directions[indX][jj] + m_startPosition.x;
				positionsY[ll] = directions[indY][kk] + m_startPosition.y;
				positionsZ[ll] = directions[indZ][ii] + m_startPosition.z;
				ll++;
			}
		}
	}

	int rank{ 3 };
	hsize_t* dims = new hsize_t[rank];
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
	
	int rank_data{ 3 };
	hsize_t dims_data[3] = { (hsize_t)m_settings.camera.frameCount, (hsize_t)m_settings.camera.roi.height, (hsize_t)m_settings.camera.roi.width };
	long long bytesPerFrame{ 2 * m_settings.camera.roi.width * m_settings.camera.roi.height };

	// reset number of calibrations
	nrCalibrations = 1;
	// do pre calibration
	if (m_settings.preCalibration) {
		calibrate(storage);
	}

	QElapsedTimer measurementTimer;
	measurementTimer.start();

	QElapsedTimer calibrationTimer;
	calibrationTimer.start();

	// move stage to first position, wait 50 ms for it to finish
	(*m_scanControl)->setPosition(orderedPositions[0]);
	Sleep(50);

	std::string binning = getBinningString();

	for (gsl::index ll{ 0 }; ll < nrPositions; ll++) {

		// do live calibration if required and possible at the moment
		if (m_settings.conCalibration && calibrationAllowed[ll]) {
			if (calibrationTimer.elapsed() > (60e3 * m_settings.conCalibrationInterval)) {
				calibrate(storage);
				calibrationTimer.start();
				// After we calibrated, we move back to the current position
				(*m_scanControl)->setPosition(orderedPositions[ll]);
				Sleep(100);
			}
		}

		int nextCalibration = 100 * (1e-3 * calibrationTimer.elapsed()) / (60 * m_settings.conCalibrationInterval);
		emit(s_timeToCalibration(nextCalibration));

		std::vector<unsigned char> images(bytesPerFrame * m_settings.camera.frameCount);

		for (gsl::index mm{ 0 }; mm < m_settings.camera.frameCount; mm++) {
			if (m_abort) {
				this->abortMode(storage);
				return;
			}
			emit(s_positionChanged(orderedPositions[ll] - m_startPosition, mm + 1));
			// acquire images
			int64_t pointerPos = (int64_t)bytesPerFrame * mm;
			(*m_andor)->getImageForAcquisition(&images[pointerPos]);
		}

		// cast the vector to unsigned short
		std::vector<unsigned short>* images_ = (std::vector<unsigned short> *) &images;

		// asynchronously write image to disk
		// the datetime has to be set here, otherwise it would be determined by the time the queue is processed
		std::string date = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
			.toString(Qt::ISODateWithMs).toStdString();
		IMAGE* img = new IMAGE(indexX[ll], indexY[ll], indexZ[ll], rank_data, dims_data, date, *images_,
			m_settings.camera.exposureTime, m_settings.camera.gain, binning);

		// move stage to next position before saving the images
		if (ll < ((gsl::index)nrPositions - 1)) {
			(*m_scanControl)->setPosition(orderedPositions[ll + 1]);
		}

		QMetaObject::invokeMethod(
			storage.get(),
			[&storage = storage, img]() { storage.get()->s_enqueuePayload(img); },
			Qt::AutoConnection
		);

		double percentage{ 100 * (double)(ll + 1) / nrPositions };
		int remaining{ (int)(1e-3 * measurementTimer.elapsed() / (ll + 1) * ((int64_t)nrPositions - ll + 1)) };
		emit(s_repetitionProgress(percentage, remaining));
	}
	// do post calibration
	if (m_settings.postCalibration) {
		calibrate(storage);
	}

	// close camera libraries, clear buffers
	(*m_andor)->stopAcquisition();

	(*m_scanControl)->setPreset(ScanPreset::SCAN_LASEROFF);

	(*m_scanControl)->setPosition(m_startPosition);
	emit(s_positionChanged({ 0, 0, 0 }, 0));
	QMetaObject::invokeMethod(
		(*m_scanControl),
		[&m_scanControl = (*m_scanControl)]() { m_scanControl->startAnnouncing(); },
		Qt::AutoConnection
	);

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

	std::string info = "Acquisition finished.";
	qInfo(logInfo()) << info.c_str();
	emit(s_calibrationRunning(false));
	setAcquisitionStatus(ACQUISITION_STATUS::FINISHED);
	emit(s_timeToCalibration(0));
}