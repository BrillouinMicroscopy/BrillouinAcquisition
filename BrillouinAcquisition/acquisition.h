#ifndef ACQUISITION_H
#define ACQUISITION_H

#include "storageWrapper.h"
#include "thread.h"
#include "andor.h"
#include "scancontrol.h"
#include "circularBuffer.h"

enum ACQUISITION_STATES {
	STARTED,
	RUNNING,
	FINISHED,
	ABORTED
};

struct REPETITIONS {
	int count = 1;			// [1]		number of repetitions
	double interval = 10;	// [min]	interval between repetitions
};

struct ACQUISITION_SETTINGS {
	std::string filename = "Brillouin.h5";	// filename
	std::string folder = ".";
	std::string fullPath = folder + filename;
	
	// calibration parameters
	std::string sample = "Methanol & Water";
	bool preCalibration = true;				// do pre calibration
	bool postCalibration = true;			// do post calibration
	bool conCalibration = true;				// do continuous calibration
	double conCalibrationInterval = 10;		// interval of continuous calibrations
	int nrCalibrationImages = 10;				// number of calibration images
	double calibrationExposureTime = 1;		// exposure time for calibration images
	
	// repetition parameters
	REPETITIONS repetitions;

	// ROI parameters
	double xMin = 0;	// [µm]	x minimum value
	double xMax = 10;	// [µm]	x maximum value
	int xSteps = 3;		// [1]	x steps
	double yMin = 0;	// [µm]	y minimum value
	double yMax = 10;	// [µm]	y maximum value
	int ySteps = 3;		// [1]	y steps
	double zMin = 0;	// [µm]	z minimum value
	double zMax = 0;	// [µm]	z maximum value
	int zSteps = 1;		// [1]	z steps

	CAMERA_SETTINGS camera;
};

struct ACQUISITION {
	ACQUISITION_SETTINGS settings;	// settings of the acquisition
	StorageWrapper *fileHndl;		// handle to the file
	bool running = true;			// is the acquisition still running
	ACQUISITION(ACQUISITION_SETTINGS settings) : settings(settings),
		fileHndl(new StorageWrapper(nullptr, settings.fullPath, H5F_ACC_RDWR)) {};
	~ACQUISITION() {
		if (fileHndl) {
			delete fileHndl;
			fileHndl = nullptr;
		}
	};
};

class Acquisition : public QObject {
	Q_OBJECT

public:
	Acquisition(QObject *parent, Andor *andor, ScanControl **scanControl);
	~Acquisition();
	bool m_abort = false;
	bool isAcqRunning();

public slots:
	void init() {};
	void startAcquisition(ACQUISITION_SETTINGS acqSettings);

private:
	ACQUISITION_SETTINGS m_acqSettings;
	//Thread m_storageThread;
	Andor *m_andor;
	ScanControl **m_scanControl;
	bool m_running = false;				// is acquisition currently running
	POINT3 m_startPosition{ 0, 0, 0 };
	void abort();
	void checkFilename();

	int nrCalibrations = 1;
	void doCalibration(ACQUISITION *acquisition);

	void runAcquisition(ACQUISITION *acquisition);

signals:
	void s_acqRunning(bool);			// is acquisition running
	void s_acqProgress(int, double, int);	// progress in percent and the remaining time in seconds
	// current position in x, y and z, as well as the current image number
	void s_acqPosition(POINT3, int);
	void s_acqTimeToCalibration(int);	// time to next calibration
	void s_acqCalibrationRunning(bool);	// is calibration running
	void s_filenameChanged(std::string);
	void s_acqRepetitionProgress(int, int);	// repetitions
};

#endif //ACQUISITION_H