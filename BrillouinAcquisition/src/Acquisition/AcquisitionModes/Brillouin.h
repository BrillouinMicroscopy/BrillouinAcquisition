#ifndef BRILLOUIN_H
#define BRILLOUIN_H

#include "AcquisitionMode.h"
#include "..\..\Devices\Cameras\Camera.h"
#include "..\..\thread.h"
#include "..\..\circularBuffer.h"


struct SCAN_ORDER {
	bool automatical{ true };
	int x{ 0 };	// first scan in x-direction
	int y{ 1 };	// then in y-direction
	int z{ 2 };	// scan in z-direction last
};

struct BRILLOUIN_SETTINGS {
	// calibration parameters
	std::string sample{ "Methanol & Water" };
	bool preCalibration{ true };				// do pre calibration
	bool postCalibration{ true };				// do post calibration
	bool conCalibration{ true };				// do continuous calibration
	double conCalibrationInterval{ 10 };		// interval of continuous calibrations
	int nrCalibrationImages{ 10 };				// number of calibration images
	double calibrationExposureTime{ 1 };		// exposure time for calibration images

	// repetition parameters
	REPETITIONS repetitions;

	// ROI parameters
	double xMin{ 0 };	// [�m]	x minimum value
	double xMax{ 10 };	// [�m]	x maximum value
	int xSteps{ 3 };	// [1]	x steps
	double yMin{ 0 };	// [�m]	y minimum value
	double yMax{ 10 };	// [�m]	y maximum value
	int ySteps{ 3 };	// [1]	y steps
	double zMin{ 0 };	// [�m]	z minimum value
	double zMax{ 0 };	// [�m]	z maximum value
	int zSteps{ 1 };	// [1]	z steps

	CAMERA_SETTINGS camera;
};

class Brillouin : public AcquisitionMode {
	Q_OBJECT

public:
	Brillouin(QObject* parent, Acquisition* acquisition, Camera** andor, ScanControl** scanControl);
	~Brillouin();

public slots:
	void startRepetitions() override;

	void waitForNextRepetition();
	void finaliseRepetitions();
	void finaliseRepetitions(int, int);

	void setStepNumberX(int);
	void setStepNumberY(int);
	void setStepNumberZ(int);

	void setXMin(double);
	void setXMax(double);
	void setYMin(double);
	void setYMax(double);
	void setZMin(double);
	void setZMax(double);

	void setSettings(const BRILLOUIN_SETTINGS& settings);

	/*
	 *	Scan direction order related variables and functions
	 */

	void setScanOrderX(int x);
	void setScanOrderY(int y);
	void setScanOrderZ(int z);

	void setScanOrderAuto(bool automatical);

	void determineScanOrder();

	std::vector<POINT3> getOrderedPositions();

private:
	void abortMode(std::unique_ptr <StorageWrapper>& storage) override;

	void calibrate(std::unique_ptr <StorageWrapper>& storage);

	std::string getRepetitionFilename();

	BRILLOUIN_SETTINGS m_settings;
	SCAN_ORDER m_scanOrder;
	Camera** m_andor{ nullptr };
	bool m_running{ false };				// is acquisition currently running
	POINT3 m_startPosition{ 0, 0, 0 };

	QTimer* m_repetitionTimer{ nullptr };
	QElapsedTimer m_startOfLastRepetition;
	int m_currentRepetition{ 0 };

	int nrCalibrations{ 1 };

	std::string m_baseFilename{ "" };

	std::vector<POINT3> m_orderedPositions;	// The positions to measure in absolute values
	std::vector<POINT3> m_orderedPositionsRelative;	// The positions to measure relative to start position
	std::vector<INDEX3> m_orderedIndices;	// The associated indices
	std::vector<bool> m_calibrationAllowed;	// If a calibration is allowed for this position

private slots:
	void acquire(std::unique_ptr <StorageWrapper>& storage) override;

	void updatePositions();

signals:
	// current position in x, y and z, as well as the current image number
	void s_positionChanged(POINT3, int);
	void s_timeToCalibration(int);	// time to next calibration
	void s_calibrationRunning(bool);	// is calibration running
	void s_scanOrderChanged(SCAN_ORDER);
	void s_orderedPositionsChanged(std::vector<POINT3>);
};

#endif //BRILLOUIN_H