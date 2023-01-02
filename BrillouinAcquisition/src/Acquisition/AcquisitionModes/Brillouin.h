#ifndef BRILLOUIN_H
#define BRILLOUIN_H

#include "AcquisitionMode.h"
#include "../../Devices/Cameras/Camera.h"
#include "../../helper/thread.h"
#include "src/lib/buffer_circular.h"


struct SCAN_ORDER {
	bool automatical{ true };
	int x{ 0 };	// first scan in x-direction
	int y{ 1 };	// then in y-direction
	int z{ 2 };	// scan in z-direction last
};

struct BRILLOUIN_SETTINGS {
	private:
		// ROI parameters
		double m_xMin{ 0 };		// [µm]	x minimum value
		double m_xMax{ 10 };	// [µm]	x maximum value
		int m_xSteps{ 3 };		// [1]	x steps
		double m_yMin{ 0 };		// [µm]	y minimum value
		double m_yMax{ 10 };	// [µm]	y maximum value
		int m_ySteps{ 3 };		// [1]	y steps
		double m_zMin{ 0 };		// [µm]	z minimum value
		double m_zMax{ 0 };		// [µm]	z maximum value
		int m_zSteps{ 1 };		// [1]	z steps

		// ROI limits
		std::tuple<double, double> m_xyzLim{ -1000000, 1000000 };
		std::tuple<int, int> m_stepsLim{ 1, 100000 };

		template <typename T>
		void checkLimits(T &value, std::tuple<T, T> limits) {
			if (value < std::get<0>(limits)) {
				value = std::get<0>(limits);
			}
			if (value > std::get<1>(limits)) {
				value = std::get<1>(limits);
			}
		};

	public:
		BRILLOUIN_SETTINGS& operator=(const BRILLOUIN_SETTINGS& settings) {
			m_xMin = settings.xMin;
			m_xMax = settings.xMax;
			m_xSteps = settings.xSteps;
			m_yMin = settings.yMin;
			m_yMax = settings.yMax;
			m_ySteps = settings.ySteps;
			m_zMin = settings.zMin;
			m_zMax = settings.zMax;
			m_zSteps = settings.zSteps;
			sample = settings.sample;
			preCalibration = settings.preCalibration;
			postCalibration = settings.postCalibration;
			conCalibration = settings.conCalibration;
			conCalibrationInterval = settings.conCalibrationInterval;
			nrCalibrationImages = settings.nrCalibrationImages;
			calibrationExposureTime = settings.calibrationExposureTime;
			repetitions = settings.repetitions;
			camera = settings.camera;
			return *this;
		}
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
		const double& xMin{ m_xMin };
		void setXMin(double xMin) {
			checkLimits(xMin, m_xyzLim);
			m_xMin = xMin;
			if (m_xMax < m_xMin) {
				m_xMax = m_xMin;
			}
		};
		const double& xMax{ m_xMax };
		void setXMax(double xMax) {
			checkLimits(xMax, m_xyzLim);
			m_xMax = xMax;
			if (m_xMax < m_xMin) {
				m_xMin = m_xMax;
			}
		};
		const int& xSteps{ m_xSteps };
		void setXSteps(int xSteps) {
			checkLimits(xSteps, m_stepsLim);
			m_xSteps = xSteps;
		};
		const double& yMin = m_yMin;
		void setYMin(double yMin) {
			checkLimits(yMin, m_xyzLim);
			m_yMin = yMin;
			if (m_yMax < m_yMin) {
				m_yMax = m_yMin;
			}
		};
		const double& yMax = m_yMax;
		void setYMax(double yMax) {
			checkLimits(yMax, m_xyzLim);
			m_yMax = yMax;
			if (m_yMax < m_yMin) {
				m_yMin = m_yMax;
			}
		};
		const int& ySteps = m_ySteps;
		void setYSteps(int ySteps) {
			checkLimits(ySteps, m_stepsLim);
			m_ySteps = ySteps;
		};
		const double& zMin = m_zMin;
		void setZMin(double zMin) {
			checkLimits(zMin, m_xyzLim);
			m_zMin = zMin;
			if (m_zMax < m_zMin) {
				m_zMax = m_zMin;
			}
		};
		const double& zMax = m_zMax;
		void setZMax(double zMax) {
			checkLimits(zMax, m_xyzLim);
			m_zMax = zMax;
			if (m_zMax < m_zMin) {
				m_zMin = m_zMax;
			}
		};
		const int& zSteps = m_zSteps;
		void setZSteps(int zSteps) {
			checkLimits(zSteps, m_stepsLim);
			m_zSteps = zSteps;
		};

		CAMERA_SETTINGS camera;
};

class Brillouin : public AcquisitionMode {
	Q_OBJECT

public:
	Brillouin(QObject* parent, Acquisition* acquisition, Camera** andor, ScanControl** scanControl);
	~Brillouin();

	BRILLOUIN_SETTINGS& settings{ m_settings };

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