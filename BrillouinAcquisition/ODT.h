#ifndef ODT_H
#define ODT_H

#include "PointGrey.h"
#include "scancontrol.h"
#include "circularBuffer.h"

enum ODT_STATES {
	ODT_STARTED,
	ODT_RUNNING,
	ODT_FINISHED,
	ODT_ABORTED
};

enum ODT_SETTING {
	VOLTAGE,
	NRPOINTS,
	SCANRATE
};

enum ODT_MODES {
	ALGN,
	ACQ
};

struct ODT_SETTINGS {
	double radialVoltage{ 0.1 };	// maximum voltage for the galvo scanners
	int numberPoints{ 30 };			// number of points
	double scanRate{ 100 };			// scan rate
	std::vector<VOLTAGE2> voltages;	// voltages to apply
	CAMERA_SETTINGS camera;
};

class ODT : public QObject {
	Q_OBJECT

public:
	ODT(QObject *parent, PointGrey **pointGrey, ScanControl **scanControl);
	~ODT();
	bool m_abortAcquisition = false;
	bool m_abortAlignment = false;
	bool isAcqRunning();
	bool isAlgnRunning();
	ODT_SETTINGS getAcqSettings();
	ODT_SETTINGS getAlgnSettings();
	void setAlgnSettings(ODT_SETTINGS);
	void setAcqSettings(ODT_SETTINGS);
	void setSettings(ODT_MODES, ODT_SETTING, double);

public slots:
	void init() {};
	void initialize();
	void startAcquisition(ODT_SETTINGS acqSettings) {};
	void startAlignment(ODT_SETTINGS algnSettings) {};

private:
	ODT_SETTINGS m_acqSettings{
		0.1,
		150,
		100,
		{},
		CAMERA_SETTINGS()
	};
	ODT_SETTINGS m_algnSettings;
	PointGrey **m_pointGrey;
	ScanControl **m_scanControl;
	bool m_acqRunning{ false };				// is acquisition currently running
	bool m_algnRunning{ false };			// is alignment currently running
	void abortAcquisition();
	void abortAlignment();

	void calculateVoltages(ODT_MODES);

signals:
	void s_acqRunning(bool);					// is acquisition running
	void s_acqSettingsChanged(ODT_SETTINGS);	// emit the acquisition voltages
	void s_acqProgress(int, double, int);		// progress in percent and the remaining time in seconds
	void s_algnRunning(bool);					// is alignment running
	void s_algnSettingsChanged(ODT_SETTINGS);	// emit the alignment voltages
};

#endif //ODT_H