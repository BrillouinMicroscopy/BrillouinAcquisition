#include "stdafx.h"
#include "acquisition.h"
#include "simplemath.h"
#include "logger.h"
#include "filesystem"

using namespace std::experimental::filesystem::v1;


Acquisition::Acquisition(QObject *parent, Andor *andor, ScanControl *scanControl)
	: QObject(parent), m_andor(andor), m_scanControl(scanControl) {
}

Acquisition::~Acquisition() {
	m_running = false;
}

bool Acquisition::isAcqRunning() {
	return m_running;
}

void Acquisition::startAcquisition(ACQUISITION_SETTINGS acqSettings) {
	m_running = true;
	m_abort = false;

	std::string info = "Acquisition started.";
	qInfo(logInfo()) << info.c_str();

	emit(s_acqRunning(m_running));

	m_acqSettings = acqSettings;

	QElapsedTimer startOfLastRepetition;
	startOfLastRepetition.start();

	for (gsl::index repNumber = 0; repNumber < m_acqSettings.repetitions.count; repNumber++) {

		if (m_abort) {
			abort();
			return;
		}

		checkFilename();

		ACQUISITION acquisition = ACQUISITION(m_acqSettings);

		if (repNumber == 0) {
			emit(s_acqRepetitionProgress(repNumber, -1));
			runAcquisition(&acquisition);
		} else {
			int timeSinceLast = startOfLastRepetition.elapsed()*1e-3;
			while (timeSinceLast < m_acqSettings.repetitions.interval * 60) {
				timeSinceLast = startOfLastRepetition.elapsed()*1e-3;
				emit(s_acqRepetitionProgress(repNumber, m_acqSettings.repetitions.interval * 60 - timeSinceLast));
				Sleep(100);
			}
			startOfLastRepetition.restart();
			emit(s_acqRepetitionProgress(repNumber, -1));
			runAcquisition(&acquisition);
		}
	}
	emit(s_acqRepetitionProgress(m_acqSettings.repetitions.count, -1));
	m_running = false;
	emit(s_acqRunning(m_running));
}

void Acquisition::runAcquisition(ACQUISITION *acquisition) {

	emit(s_acqProgress(ACQUISITION_STATES::STARTED, 0.0, -1));
	
	// prepare camera for image acquisition
	acquisition->settings.camera = m_andor->prepareMeasurement(acquisition->settings.camera);
	// set optical elements for brightfield/Brillouin imaging
	m_scanControl->m_stand->setPreset(1);
	Sleep(500);

	// get current stage position
	m_startPosition = m_scanControl->getPosition();

	std::string now = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
		.toString(Qt::ISODateWithMs).toStdString();

	acquisition->fileHndl->setDate(now);

	std::string commentIn = "Brillouin data";
	acquisition->fileHndl->setComment(commentIn);

	acquisition->fileHndl->setResolution("x", acquisition->settings.xSteps);
	acquisition->fileHndl->setResolution("y", acquisition->settings.ySteps);
	acquisition->fileHndl->setResolution("z", acquisition->settings.zSteps);

	int resolutionXout = acquisition->fileHndl->getResolution("x");

	// Create position vector
	int nrPositions = acquisition->settings.xSteps * acquisition->settings.ySteps * acquisition->settings.zSteps;
	std::vector<double> positionsX(nrPositions);
	std::vector<double> positionsY(nrPositions);
	std::vector<double> positionsZ(nrPositions);
	std::vector<double> posX = simplemath::linspace(acquisition->settings.xMin, acquisition->settings.xMax, acquisition->settings.xSteps);
	std::vector<double> posY = simplemath::linspace(acquisition->settings.yMin, acquisition->settings.yMax, acquisition->settings.ySteps);
	std::vector<double> posZ = simplemath::linspace(acquisition->settings.zMin, acquisition->settings.zMax, acquisition->settings.zSteps);
	int ll = 0;
	for (gsl::index ii = 0; ii < acquisition->settings.zSteps; ii++) {
		for (gsl::index jj = 0; jj < acquisition->settings.xSteps; jj++) {
			for (gsl::index kk = 0; kk < acquisition->settings.ySteps; kk++) {
				// calculate stage positions
				positionsX[ll] = posX[jj] + m_startPosition[0];
				positionsY[ll] = posY[kk] + m_startPosition[1];
				positionsZ[ll] = posZ[ii] + m_startPosition[2];
				ll++;
			}
		}
	}

	int rank = 3;
	// For compatibility with MATLAB respect Fortran-style ordering: z, x, y
	hsize_t *dims = new hsize_t[rank];
	dims[0] = acquisition->settings.zSteps;
	dims[1] = acquisition->settings.xSteps;
	dims[2] = acquisition->settings.ySteps;

	acquisition->fileHndl->setPositions("x", positionsX, rank, dims);
	acquisition->fileHndl->setPositions("y", positionsY, rank, dims);
	acquisition->fileHndl->setPositions("z", positionsZ, rank, dims);
	delete[] dims;

	// do actual measurement
	acquisition->fileHndl->startWritingQueues();
	
	int rank_data = 3;
	hsize_t dims_data[3] = { acquisition->settings.camera.frameCount, acquisition->settings.camera.roi.height, acquisition->settings.camera.roi.width };
	int bytesPerFrame = m_acqSettings.camera.roi.width * m_acqSettings.camera.roi.height * 2;
	ll = 0;

	// reset number of calibrations
	nrCalibrations = 1;
	// do pre calibration
	if (acquisition->settings.preCalibration) {
		doCalibration(acquisition);
	}

	QElapsedTimer measurementTimer;
	measurementTimer.start();

	QElapsedTimer calibrationTimer;
	calibrationTimer.start();

	for (gsl::index ii = 0; ii < acquisition->settings.zSteps; ii++) {
		for (gsl::index jj = 0; jj < acquisition->settings.xSteps; jj++) {

			// do live calibration 
			if (acquisition->settings.conCalibration) {
				if (calibrationTimer.elapsed() > (60e3 * acquisition->settings.conCalibrationInterval)) {
					doCalibration(acquisition);
					calibrationTimer.start();
				}
			}

			for (gsl::index kk = 0; kk < acquisition->settings.ySteps; kk++) {
				int nextCalibration = 100 * (1e-3 * calibrationTimer.elapsed()) / (60 * acquisition->settings.conCalibrationInterval);
				emit(s_acqTimeToCalibration(nextCalibration));
				// move stage to correct position, wait 50 ms for it to finish
				m_scanControl->setPosition({ positionsX[ll], positionsY[ll], positionsZ[ll] });

				std::vector<AT_U8> images(bytesPerFrame * acquisition->settings.camera.frameCount);

				for (gsl::index mm = 0; mm < acquisition->settings.camera.frameCount; mm++) {
					if (m_abort) {
						abort();
						return;
					}
					emit(s_acqPosition(positionsX[ll] - m_startPosition[0], positionsY[ll] - m_startPosition[1], positionsZ[ll] - m_startPosition[2], mm+1));
					// acquire images
					int64_t pointerPos = (int64_t)bytesPerFrame * mm;
					m_andor->getImageForMeasurement(&images[pointerPos]);
				}

				// cast the vector to unsigned short
				std::vector<unsigned short> *images_ = (std::vector<unsigned short> *) &images;

				// asynchronously write image to disk
				// the datetime has to be set here, otherwise it would be determined by the time the queue is processed
				std::string date = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
					.toString(Qt::ISODateWithMs).toStdString();
				IMAGE *img = new IMAGE(jj, kk, ii, rank_data, dims_data, date, *images_);

				QMetaObject::invokeMethod(acquisition->fileHndl, "s_enqueuePayload", Qt::AutoConnection, Q_ARG(IMAGE*, img));

				// increase position index
				ll++;
				double percentage = 100 * (double)ll / nrPositions;
				int remaining = 1e-3 * measurementTimer.elapsed() / ll * ((int64_t)nrPositions - ll);
				emit(s_acqProgress(ACQUISITION_STATES::RUNNING, percentage, remaining));
			}
		}
	}
	// do post calibration
	if (acquisition->settings.postCalibration) {
		doCalibration(acquisition);
	}

	// close camera libraries, clear buffers
	m_andor->stopMeasurement();

	QMetaObject::invokeMethod(acquisition->fileHndl, "s_finishedQueueing", Qt::AutoConnection);

	m_scanControl->setPosition(m_startPosition);
	emit(s_acqPosition(0, 0, 0, 0));

	std::string info = "Acquisition finished.";
	qInfo(logInfo()) << info.c_str();
	emit(s_acqCalibrationRunning(false));
	emit(s_acqProgress(ACQUISITION_STATES::FINISHED, 100.0, 0));
	emit(s_acqTimeToCalibration(0));
}

void Acquisition::abort() {
	m_andor->stopMeasurement();
	m_scanControl->setPosition(m_startPosition);
	m_running = false;
	emit(s_acqRunning(m_running));
	emit(s_acqProgress(ACQUISITION_STATES::ABORTED, 0, 0));
	emit(s_acqPosition(m_startPosition[0], m_startPosition[1], m_startPosition[2], 0));
	emit(s_acqTimeToCalibration(0));
}

void Acquisition::checkFilename() {
	std::string oldFilename = m_acqSettings.filename;
	// get filename without extension
	std::string rawFilename = oldFilename.substr(0, oldFilename.find_last_of("."));
	// remove possibly attached number separated by a hyphen
	rawFilename = rawFilename.substr(0, rawFilename.find_last_of("-"));
	m_acqSettings.fullPath = m_acqSettings.folder + "/" + m_acqSettings.filename;
	int count = 0;
	while (exists(m_acqSettings.fullPath)) {
		m_acqSettings.filename = rawFilename + '-' + std::to_string(count) + oldFilename.substr(oldFilename.find_last_of("."), std::string::npos);
		m_acqSettings.fullPath = m_acqSettings.folder + "/" + m_acqSettings.filename;
		count++;
	}
	if (m_acqSettings.filename != oldFilename) {
		emit(s_filenameChanged(m_acqSettings.filename));
	}
}

void Acquisition::doCalibration(ACQUISITION *acquisition) {
	// announce calibration start
	emit(s_acqCalibrationRunning(true));

	// set exposure time for calibration
	m_andor->setCalibrationExposureTime(acquisition->settings.calibrationExposureTime);

	// move optical elements to position for calibration
	m_scanControl->m_stand->setPreset(3);
	Sleep(500);

	double shift = 5.088; // this is the shift for water

	// acquire images
	int rank_cal = 3;
	hsize_t dims_cal[3] = { acquisition->settings.nrCalibrationImages, acquisition->settings.camera.roi.height, acquisition->settings.camera.roi.width };

	int bytesPerFrame = acquisition->settings.camera.roi.width * acquisition->settings.camera.roi.height * 2;
	std::vector<AT_U8> images((int64_t)bytesPerFrame * acquisition->settings.nrCalibrationImages);
	for (gsl::index mm = 0; mm < acquisition->settings.nrCalibrationImages; mm++) {
		if (m_abort) {
			abort();
			return;
		}
		// acquire images
		int64_t pointerPos = (int64_t)bytesPerFrame * mm;
		m_andor->getImageForMeasurement(&images[pointerPos]);
	}
	// cast the vector to unsigned short
	std::vector<unsigned short> *images_ = (std::vector<unsigned short> *) &images;

	// the datetime has to be set here, otherwise it would be determined by the time the queue is processed
	std::string date = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
		.toString(Qt::ISODateWithMs).toStdString();
	CALIBRATION *cal = new CALIBRATION(
		nrCalibrations,			// index
		*images_,				// data
		rank_cal,				// the rank of the calibration data
		dims_cal,				// the dimension of the calibration data
		acquisition->settings.sample,	// the samplename
		shift,					// the Brillouin shift of the sample
		date					// the datetime
	);
	QMetaObject::invokeMethod(acquisition->fileHndl, "s_enqueueCalibration", Qt::AutoConnection, Q_ARG(CALIBRATION*, cal));
	nrCalibrations++;

	// revert optical elements to position for brightfield/Brillouin imaging
	m_scanControl->m_stand->setPreset(1);

	// reset exposure time
	m_andor->setCalibrationExposureTime(acquisition->settings.camera.exposureTime);
	Sleep(500);
}
