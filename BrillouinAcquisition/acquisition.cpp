#include "stdafx.h"
#include "acquisition.h"
#include "simplemath.h"
#include "logger.h"


Acquisition::Acquisition(QObject *parent, Andor *andor, ScanControl *scanControl)
	: QObject(parent), m_andor(andor), m_scanControl(scanControl) {
}

Acquisition::~Acquisition() {
}

bool Acquisition::isAcqRunning() {
	return m_running;
}

void Acquisition::startAcquisition(ACQUISITION_SETTINGS acqSettings) {
	setSettings(acqSettings);

	m_running = true;
	emit(s_acqRunning(m_running));
	m_abort = false;

	emit(s_acqProgress(0.0, -1));
	// set optical elements for brightfield/Brillouin imaging
	m_scanControl->m_stand->setPreset(1);

	// get current stage position
	m_startPosition = m_scanControl->getPosition();

	m_fileHndl = new StorageWrapper(nullptr, m_acqSettings.filename, H5F_ACC_RDWR);
	// move h5bm file to separate thread
	m_storageThread.startWorker(m_fileHndl);

	std::string now = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
		.toString(Qt::ISODate).toStdString();

	m_fileHndl->setDate(now);

	std::string commentIn = "Brillouin data";
	m_fileHndl->setComment(commentIn);

	m_fileHndl->setResolution("x", m_acqSettings.xSteps);
	m_fileHndl->setResolution("y", m_acqSettings.ySteps);
	m_fileHndl->setResolution("z", m_acqSettings.zSteps);

	int resolutionXout = m_fileHndl->getResolution("x");

	// Create position vector
	int nrPositions = m_acqSettings.xSteps * m_acqSettings.ySteps * m_acqSettings.zSteps;
	std::vector<double> positionsX(nrPositions);
	std::vector<double> positionsY(nrPositions);
	std::vector<double> positionsZ(nrPositions);
	std::vector<double> posX = simplemath::linspace(m_acqSettings.xMin, m_acqSettings.xMax, m_acqSettings.xSteps);
	std::vector<double> posY = simplemath::linspace(m_acqSettings.yMin, m_acqSettings.yMax, m_acqSettings.ySteps);
	std::vector<double> posZ = simplemath::linspace(m_acqSettings.zMin, m_acqSettings.zMax, m_acqSettings.zSteps);
	int ll = 0;
	for (int ii = 0; ii < m_acqSettings.zSteps; ii++) {
		for (int jj = 0; jj < m_acqSettings.xSteps; jj++) {
			for (int kk = 0; kk < m_acqSettings.ySteps; kk++) {
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
	dims[0] = m_acqSettings.zSteps;
	dims[1] = m_acqSettings.xSteps;
	dims[2] = m_acqSettings.ySteps;

	m_fileHndl->setPositions("x", positionsX, rank, dims);
	m_fileHndl->setPositions("y", positionsY, rank, dims);
	m_fileHndl->setPositions("z", positionsZ, rank, dims);
	delete[] dims;

	// do actual measurement 
	m_fileHndl->startWritingQueues();
	// prepare camera for image acquisition
	m_andor->prepareMeasurement(m_acqSettings.camera);
	
	int rank_data = 3;
	hsize_t dims_data[3] = { m_acqSettings.camera.frameCount, m_acqSettings.camera.roi.height, m_acqSettings.camera.roi.width };
	ll = 0;

	// do pre calibration
	if (m_acqSettings.preCalibration) {
		doCalibration();
	}

	QElapsedTimer measurementTimer;
	measurementTimer.start();

	QElapsedTimer calibrationTimer;
	calibrationTimer.start();

	for (int ii = 0; ii < m_acqSettings.zSteps; ii++) {
		for (int jj = 0; jj < m_acqSettings.xSteps; jj++) {

			// do live calibration 
			if (m_acqSettings.conCalibration) {
				if (calibrationTimer.elapsed() > (60e3 * m_acqSettings.conCalibrationInterval)) {
					doCalibration();
					calibrationTimer.start();
				}
			}

			for (int kk = 0; kk < m_acqSettings.ySteps; kk++) {
				int nextCalibration = 100 * (1e-3 * calibrationTimer.elapsed()) / (60 * m_acqSettings.conCalibrationInterval);
				emit(s_acqTimeToCalibration(nextCalibration));
				// move stage to correct position, wait 50 ms for it to finish
				m_scanControl->setPosition({ positionsX[ll], positionsY[ll], positionsZ[ll] });
				Sleep(50);

				int bytesPerFrame = m_acqSettings.camera.roi.width * m_acqSettings.camera.roi.height * 2;

				std::vector<AT_U8> images(bytesPerFrame * m_acqSettings.camera.frameCount);

				for (int mm = 0; mm < m_acqSettings.camera.frameCount; mm++) {
					if (m_abort) {
						abort();
						return;
					}
					emit(s_acqPosition(positionsX[ll], positionsY[ll], positionsZ[ll], mm+1));
					// acquire images
					m_andor->acquireImage(&images[bytesPerFrame * mm]);
				}

				// cast the vector to unsigned short
				std::vector<unsigned short> *images_ = (std::vector<unsigned short> *) &images;

				// asynchronously write image to disk
				// the datetime has to be set here, otherwise it would be determined by the time the queue is processed
				std::string date = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
					.toString(Qt::ISODate).toStdString();
				IMAGE *img = new IMAGE(jj, kk, ii, rank_data, dims_data, date, *images_);
				m_fileHndl->m_payloadQueue.enqueue(img);

				std::string info = "Images acquired " + std::to_string(ii*(m_acqSettings.xSteps*m_acqSettings.ySteps) + jj * m_acqSettings.ySteps + kk);
				qInfo(logInfo()) << info.c_str();
				// increase position index
				ll++;
				emit(s_acqProgress(100 * (double)ll / nrPositions, 1e-3 * measurementTimer.elapsed() / ll * (nrPositions - ll)));
			}
		}
	}
	// do post calibration
	if (m_acqSettings.preCalibration) {
		doCalibration();
	}

	// close camera libraries, clear buffers
	m_andor->cleanupAcquisition();

	//m_scanControl->setPosition(m_startPosition);
	emit(s_acqPosition(m_startPosition[0], m_startPosition[1], m_startPosition[2], 0));

	m_storageThread.exit();
	m_storageThread.wait();
	delete m_fileHndl;
	m_fileHndl = nullptr;

	std::string info = "Acquisition finished.";
	qInfo(logInfo()) << info.c_str();
	m_running = false;
	emit(s_acqRunning(m_running));
	emit(s_acqCalibrationRunning(false));
	emit(s_acqProgress(100.0, 0));
	emit(s_acqTimeToCalibration(0));
}

void Acquisition::abort() {
	m_fileHndl->m_payloadQueue.clear();
	m_andor->cleanupAcquisition();
	m_scanControl->setPosition(m_startPosition);
	m_storageThread.exit();
	m_storageThread.wait();
	delete m_fileHndl;
	m_fileHndl = nullptr;
	m_running = false;
	emit(s_acqRunning(m_running));
	emit(s_acqProgress(0, -2));
	emit(s_acqPosition(m_startPosition[0], m_startPosition[1], m_startPosition[2], 0));
	emit(s_acqTimeToCalibration(0));
}

void Acquisition::setSettings(ACQUISITION_SETTINGS acqSettings) {
	m_acqSettings = acqSettings;

	std::string newFilename = checkFilename(acqSettings.filename);
	if (newFilename != acqSettings.filename) {
		m_acqSettings.filename = newFilename;
		emit(s_filenameChanged(newFilename));
	}
}

std::string Acquisition::checkFilename(std::string oldFilename) {
	// todo
	// if (file with old filename already exists) { add increasing counter to filename };
	return oldFilename;
}

void Acquisition::doCalibration() {
	// announce calibration start
	emit(s_acqCalibrationRunning(true));

	// set exposure time for calibration
	m_andor->setCalibrationExposureTime(m_acqSettings.calibrationExposureTime);

	// move optical elements to position for calibration
	m_scanControl->m_stand->setPreset(3);

	double shift = 5.088; // this is the shift for water

	// acquire images
	int rank_cal = 3;
	hsize_t dims_cal[3] = { m_acqSettings.nrCalibrationImages, m_acqSettings.camera.roi.height, m_acqSettings.camera.roi.width };

	int bytesPerFrame = m_acqSettings.camera.roi.width * m_acqSettings.camera.roi.height * 2;
	std::vector<AT_U8> images(bytesPerFrame * m_acqSettings.nrCalibrationImages);
	for (int mm = 0; mm < m_acqSettings.nrCalibrationImages; mm++) {
		if (m_abort) {
			abort();
			return;
		}
		// acquire images
		m_andor->acquireImage(&images[bytesPerFrame * mm]);
	}
	// cast the vector to unsigned short
	std::vector<unsigned short> *images_ = (std::vector<unsigned short> *) &images;

	// the datetime has to be set here, otherwise it would be determined by the time the queue is processed
	std::string date = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
		.toString(Qt::ISODate).toStdString();
	CALIBRATION *cal = new CALIBRATION(
		nrCalibrations,			// index
		*images_,				// data
		rank_cal,				// the rank of the calibration data
		dims_cal,				// the dimension of the calibration data
		m_acqSettings.sample,	// the samplename
		shift,					// the Brillouin shift of the sample
		date					// the datetime
	);
	m_fileHndl->m_calibrationQueue.enqueue(cal);
	nrCalibrations++;

	// revert optical elements to position for brightfield/Brillouin imaging
	m_scanControl->m_stand->setPreset(1);

	// reset exposure time
	m_andor->setCalibrationExposureTime(m_acqSettings.camera.exposureTime);
}
