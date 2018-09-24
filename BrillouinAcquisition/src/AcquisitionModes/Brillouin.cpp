#include "stdafx.h"
#include "Brillouin.h"
#include "..\simplemath.h"
#include "..\logger.h"
#include "filesystem"

using namespace std::experimental::filesystem::v1;


Brillouin::Brillouin(QObject *parent, Acquisition *acquisition, Andor *andor, ScanControl **scanControl)
	: AcquisitionMode(parent, acquisition), m_andor(andor), m_scanControl(scanControl) {
}

Brillouin::~Brillouin() {
}

void Brillouin::setSettings(BRILLOUIN_SETTINGS settings) {
	m_settings = settings;
}

void Brillouin::startRepetitions() {
	bool allowed = m_acquisition->startMode(ACQUISITION_MODE::BRILLOUIN);
	if (!allowed) {
		return;
	}

	m_abort = false;
	
	std::string info = "Acquisition started.";
	qInfo(logInfo()) << info.c_str();

	QElapsedTimer startOfLastRepetition;
	startOfLastRepetition.start();

	for (gsl::index repNumber = 0; repNumber < m_settings.repetitions.count; repNumber++) {

		if (m_abort) {
			this->abortMode();
			return;
		}

		m_acquisition->newRepetition(ACQUISITION_MODE::BRILLOUIN);

		if (repNumber == 0) {
			emit(s_totalProgress(repNumber, -1));
			acquire(m_acquisition->m_storage);
		} else {
			int timeSinceLast = startOfLastRepetition.elapsed()*1e-3;
			while (timeSinceLast < m_settings.repetitions.interval * 60) {
				timeSinceLast = startOfLastRepetition.elapsed()*1e-3;
				emit(s_totalProgress(repNumber, m_settings.repetitions.interval * 60 - timeSinceLast));
				Sleep(100);
			}
			startOfLastRepetition.restart();
			emit(s_totalProgress(repNumber, -1));
			acquire(m_acquisition->m_storage);
		}
	}
	emit(s_totalProgress(m_settings.repetitions.count, -1));
	m_acquisition->stopMode(ACQUISITION_MODE::BRILLOUIN);
}

void Brillouin::acquire(std::unique_ptr <StorageWrapper> & storage) {

	emit(s_repetitionProgress(ACQUISITION_STATE::STARTED, 0.0, -1));
	
	// prepare camera for image acquisition
	m_andor->startAcquisition(m_settings.camera);
	m_settings.camera = m_andor->getSettings();
	(*m_scanControl)->stopAnnouncingPosition();
	// set optical elements for brightfield/Brillouin imaging
	(*m_scanControl)->setElements(ScanControl::SCAN_BRIGHTFIELD);
	Sleep(500);

	// get current stage position
	m_startPosition = (*m_scanControl)->getPosition();

	std::string commentIn = "Brillouin data";
	storage->setComment(commentIn);

	storage->setResolution("x", m_settings.xSteps);
	storage->setResolution("y", m_settings.ySteps);
	storage->setResolution("z", m_settings.zSteps);

	int resolutionXout = storage->getResolution("x");

	// Create position vector
	int nrPositions = m_settings.xSteps * m_settings.ySteps * m_settings.zSteps;
	std::vector<double> positionsX(nrPositions);
	std::vector<double> positionsY(nrPositions);
	std::vector<double> positionsZ(nrPositions);
	std::vector<double> posX = simplemath::linspace(m_settings.xMin, m_settings.xMax, m_settings.xSteps);
	std::vector<double> posY = simplemath::linspace(m_settings.yMin, m_settings.yMax, m_settings.ySteps);
	std::vector<double> posZ = simplemath::linspace(m_settings.zMin, m_settings.zMax, m_settings.zSteps);
	int ll = 0;
	for (gsl::index ii = 0; ii < m_settings.zSteps; ii++) {
		for (gsl::index jj = 0; jj < m_settings.xSteps; jj++) {
			for (gsl::index kk = 0; kk < m_settings.ySteps; kk++) {
				// calculate stage positions
				positionsX[ll] = posX[jj] + m_startPosition.x;
				positionsY[ll] = posY[kk] + m_startPosition.y;
				positionsZ[ll] = posZ[ii] + m_startPosition.z;
				ll++;
			}
		}
	}

	int rank = 3;
	// For compatibility with MATLAB respect Fortran-style ordering: z, x, y
	hsize_t *dims = new hsize_t[rank];
	dims[0] = m_settings.zSteps;
	dims[1] = m_settings.xSteps;
	dims[2] = m_settings.ySteps;

	storage->setPositions("x", positionsX, rank, dims);
	storage->setPositions("y", positionsY, rank, dims);
	storage->setPositions("z", positionsZ, rank, dims);
	delete[] dims;

	// do actual measurement
	storage->startWritingQueues();
	
	int rank_data = 3;
	hsize_t dims_data[3] = { m_settings.camera.frameCount, m_settings.camera.roi.height, m_settings.camera.roi.width };
	int bytesPerFrame = m_settings.camera.roi.width * m_settings.camera.roi.height * 2;
	ll = 0;

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

	for (gsl::index ii = 0; ii < m_settings.zSteps; ii++) {
		for (gsl::index jj = 0; jj < m_settings.xSteps; jj++) {

			// do live calibration 
			if (m_settings.conCalibration) {
				if (calibrationTimer.elapsed() > (60e3 * m_settings.conCalibrationInterval)) {
					calibrate(storage);
					calibrationTimer.start();
				}
			}

			for (gsl::index kk = 0; kk < m_settings.ySteps; kk++) {
				int nextCalibration = 100 * (1e-3 * calibrationTimer.elapsed()) / (60 * m_settings.conCalibrationInterval);
				emit(s_timeToCalibration(nextCalibration));
				// move stage to correct position, wait 50 ms for it to finish
				POINT3 newPosition{ positionsX[ll], positionsY[ll], positionsZ[ll] };
				(*m_scanControl)->setPosition(newPosition);

				std::vector<AT_U8> images(bytesPerFrame * m_settings.camera.frameCount);

				for (gsl::index mm = 0; mm < m_settings.camera.frameCount; mm++) {
					if (m_abort) {
						this->abortMode();
						return;
					}
					emit(s_positionChanged( newPosition - m_startPosition, mm + 1));
					// acquire images
					int64_t pointerPos = (int64_t)bytesPerFrame * mm;
					m_andor->getImageForAcquisition(&images[pointerPos]);
				}

				// cast the vector to unsigned short
				std::vector<unsigned short> *images_ = (std::vector<unsigned short> *) &images;

				// asynchronously write image to disk
				// the datetime has to be set here, otherwise it would be determined by the time the queue is processed
				std::string date = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
					.toString(Qt::ISODateWithMs).toStdString();
				IMAGE *img = new IMAGE(jj, kk, ii, rank_data, dims_data, date, *images_);

				QMetaObject::invokeMethod(storage.get(), "s_enqueuePayload", Qt::AutoConnection, Q_ARG(IMAGE*, img));

				// increase position index
				ll++;
				double percentage = 100 * (double)ll / nrPositions;
				int remaining = 1e-3 * measurementTimer.elapsed() / ll * ((int64_t)nrPositions - ll);
				emit(s_repetitionProgress(ACQUISITION_STATE::RUNNING, percentage, remaining));
			}
		}
	}
	// do post calibration
	if (m_settings.postCalibration) {
		calibrate(storage);
	}

	// close camera libraries, clear buffers
	m_andor->stopAcquisition();

	QMetaObject::invokeMethod(storage.get(), "s_finishedQueueing", Qt::AutoConnection);

	(*m_scanControl)->setPosition(m_startPosition);
	emit(s_positionChanged({ 0, 0, 0 }, 0));
	(*m_scanControl)->startAnnouncingPosition();

	std::string info = "Acquisition finished.";
	qInfo(logInfo()) << info.c_str();
	emit(s_calibrationRunning(false));
	emit(s_repetitionProgress(ACQUISITION_STATE::FINISHED, 100.0, 0));
	emit(s_timeToCalibration(0));
}

void Brillouin::abortMode() {
	m_andor->stopAcquisition();
	(*m_scanControl)->setPosition(m_startPosition);
	m_acquisition->stopMode(ACQUISITION_MODE::BRILLOUIN);
	emit(s_repetitionProgress(ACQUISITION_STATE::ABORTED, 0, 0));
	emit(s_positionChanged(m_startPosition, 0));
	emit(s_timeToCalibration(0));
}

void Brillouin::calibrate(std::unique_ptr <StorageWrapper> & storage) {
	// announce calibration start
	emit(s_calibrationRunning(true));

	// set exposure time for calibration
	m_andor->setCalibrationExposureTime(m_settings.calibrationExposureTime);

	// move optical elements to position for calibration
	(*m_scanControl)->setElements(ScanControl::SCAN_CALIBRATION);
	Sleep(500);

	double shift = 5.088; // this is the shift for water

	// acquire images
	int rank_cal = 3;
	hsize_t dims_cal[3] = { m_settings.nrCalibrationImages, m_settings.camera.roi.height, m_settings.camera.roi.width };

	int bytesPerFrame = m_settings.camera.roi.width * m_settings.camera.roi.height * 2;
	std::vector<AT_U8> images((int64_t)bytesPerFrame * m_settings.nrCalibrationImages);
	for (gsl::index mm = 0; mm < m_settings.nrCalibrationImages; mm++) {
		if (m_abort) {
			this->abortMode();
			return;
		}
		// acquire images
		int64_t pointerPos = (int64_t)bytesPerFrame * mm;
		m_andor->getImageForAcquisition(&images[pointerPos]);
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
		m_settings.sample,		// the samplename
		shift,					// the Brillouin shift of the sample
		date					// the datetime
	);

	QMetaObject::invokeMethod(storage.get(), "s_enqueueCalibration", Qt::AutoConnection, Q_ARG(CALIBRATION*, cal));

	nrCalibrations++;

	// revert optical elements to position for brightfield/Brillouin imaging
	(*m_scanControl)->setElements(ScanControl::SCAN_BRIGHTFIELD);

	// reset exposure time
	m_andor->setCalibrationExposureTime(m_settings.camera.exposureTime);
	Sleep(500);
}
