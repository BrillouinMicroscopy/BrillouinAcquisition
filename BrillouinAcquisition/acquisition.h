#ifndef ACQUISITION_H
#define ACQUISITION_H

#include "storageWrapper.h"
#include "thread.h"
#include "andor.h"
#include "scancontrol.h"


struct ACQUISITION_SETTINGS {
	std::string filename = "Brillouin.h5";	// filename
	
	// calibration parameters
	std::string sample = "Methanol & Water";
	bool preCalibration = TRUE;				// do pre calibration
	bool postCalibration = TRUE;			// do post calibration
	bool conCalibration = TRUE;				// do continuous calibration
	double conCalibrationInterval = 10;		// interval of continuous calibrations
	int calibrationImages = 10;				// number of calibration images
	double calibrationExposureTime = 1;		// exposure time for calibration images
											// repetition parameters
	int repetitionCount = 1;				// number of repetitions
	double repetitionInterval = 10;			// repetition interval

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

class Acquisition : public QObject {
	Q_OBJECT

public:
	Acquisition(QObject *parent, Andor *andor, ScanControl *scanControl);
	~Acquisition();
	bool m_abort = 0;
	bool isAcqRunning();

public slots:
	void startAcquisition(std::string filename = "Brillouin.h5");

private:
	ACQUISITION_SETTINGS m_acqSettings;
	Thread m_storageThread;
	StorageWrapper *m_fileHndl;			// file handle
	Andor *m_andor;
	ScanControl *m_scanControl;
	bool m_running = 0;				// is acquisition currently running
	void abort(std::vector<double> startPosition);

signals:
	void s_acqRunning(bool);			// is acquisition running
	void s_acqProgress(double, int);	// progress in percent and the remaining time in seconds
	// current position in x, y and z, as well as the current image number
	void s_acqPosition(double, double, double, int);
	void s_acqTimeToCalibration(int);	// time to next calibration
	void s_acqCalibrationRunning(bool);	// is calibration running
};

#endif //ACQUISITION_H