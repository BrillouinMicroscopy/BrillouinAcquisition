#ifndef BRILLOUIN_H
#define BRILLOUIN_H

#include "AcquisitionMode.h"
#include "../../Devices/andor.h"
#include "../../Devices/scancontrol.h"
#include "../../thread.h"
#include "../../circularBuffer.h"


struct SCAN_ORDER {
	bool automatical{ true };
	int x{ 0 };	// first scan in x-direction
	int y{ 1 };	// then in y-direction
	int z{ 2 };	// scan in z-direction last
};

struct BRILLOUIN_SETTINGS {
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

class Brillouin : public AcquisitionMode {
	Q_OBJECT

public:
	Brillouin(QObject* parent, Acquisition* acquisition, Camera* andor, ScanControl** scanControl);
	~Brillouin();

public slots:
	void init() {};
	void startRepetitions();

	void waitForNextRepetition();
	void finaliseRepetitions();
	void finaliseRepetitions(int, int);

	void setStepNumberX(int);
	void setStepNumberY(int);
	void setStepNumberZ(int);

	void setSettings(BRILLOUIN_SETTINGS settings);

	/*
	 *	Scan direction order related variables and functions
	 */

	int getScanOrderX();
	int getScanOrderY();
	int getScanOrderZ();

	void setScanOrderX(int x);
	void setScanOrderY(int y);
	void setScanOrderZ(int z);

	void setScanOrderAuto(bool automatical);

	void determineScanOrder();

	void getScanOrder();

private:
	BRILLOUIN_SETTINGS m_settings;
	SCAN_ORDER m_scanOrder;
	Camera* m_andor;
	ScanControl** m_scanControl;
	bool m_running = false;				// is acquisition currently running
	POINT3 m_startPosition{ 0, 0, 0 };

	QTimer *m_repetitionTimer = nullptr;
	QElapsedTimer m_startOfLastRepetition;
	int m_currentRepetition{ 0 };

	int nrCalibrations = 1;
	void calibrate(std::unique_ptr <StorageWrapper>& storage);

	void abortMode(std::unique_ptr <StorageWrapper>& storage) override;

private slots:
	void acquire(std::unique_ptr <StorageWrapper>& storage) override;

signals:
	// current position in x, y and z, as well as the current image number
	void s_positionChanged(POINT3, int);
	void s_timeToCalibration(int);	// time to next calibration
	void s_calibrationRunning(bool);	// is calibration running
	void s_scanOrderChanged(SCAN_ORDER);
};

#endif //BRILLOUIN_H