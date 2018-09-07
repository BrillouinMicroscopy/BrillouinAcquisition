#ifndef ODT_H
#define ODT_H

#include "PointGrey.h"
#include "NIDAQ.h"
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
	double radialVoltage{ 0.1 };	// [V]	maximum voltage for the galvo scanners
	int numberPoints{ 30 };			// [1]	number of points
	double scanRate{ 1 };			// [Hz]	scan rate, for alignment: rate for one rotation, for acquisition: rate for one step
	std::vector<VOLTAGE2> voltages;	// [V]	voltages to apply
	CAMERA_SETTINGS camera;
};

class ODT : public QObject {
	Q_OBJECT

public:
	ODT(QObject *parent, PointGrey **pointGrey, NIDAQ **nidaq);
	~ODT();
	bool m_abortAcquisition = false;
	bool m_abortAlignment = false;
	bool isAcqRunning();
	bool isAlgnRunning();
	void abortAcquisition();
	ODT_SETTINGS getAcqSettings();
	ODT_SETTINGS getAlgnSettings();
	void setAlgnSettings(ODT_SETTINGS);
	void setAcqSettings(ODT_SETTINGS);

public slots:
	void init();
	void initialize();
	void startAcquisition();
	void startAlignment();
	void setSettings(ODT_MODES, ODT_SETTING, double);

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
	NIDAQ **m_NIDAQ;
	bool m_acqRunning{ false };				// is acquisition currently running
	bool m_algnRunning{ false };			// is alignment currently running

	int m_algnPositionIndex{ 0 };

	QTimer *m_algnTimer = nullptr;

	void calculateVoltages(ODT_MODES);

private slots:
	void acquisition();
	void nextAlgnPosition();

signals:
	void s_acqRunning(bool);					// is acquisition running
	void s_acqSettingsChanged(ODT_SETTINGS);	// emit the acquisition voltages
	void s_acqProgress(int, double, int);		// progress in percent and the remaining time in seconds
	void s_algnRunning(bool);					// is alignment running
	void s_algnSettingsChanged(ODT_SETTINGS);	// emit the alignment voltages
	void s_mirrorVoltageChanged(VOLTAGE2, ODT_MODES);		// emit the mirror voltage
};

#endif //ODT_H