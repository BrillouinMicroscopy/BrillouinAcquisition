#ifndef ODT_H
#define ODT_H

#include "AcquisitionMode.h"
#include "PointGrey.h"
#include "NIDAQ.h"
#include "circularBuffer.h"

enum class ODT_SETTING {
	VOLTAGE,
	NRPOINTS,
	SCANRATE
};

enum class ODT_MODE {
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

class ODT : public AcquisitionMode {
	Q_OBJECT

public:
	ODT(QObject *parent, Acquisition *acquisition, PointGrey **pointGrey, NIDAQ **nidaq);
	~ODT();
	bool m_abortAlignment = false;
	bool isAlgnRunning();
	ODT_SETTINGS getAcqSettings();
	ODT_SETTINGS getAlgnSettings();
	void setAlgnSettings(ODT_SETTINGS);
	void setSettings(ODT_SETTINGS);

public slots:
	void init();
	void initialize();
	void startRepetitions();
	void startAlignment();
	void setSettings(ODT_MODE, ODT_SETTING, double);

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
	bool m_algnRunning{ false };			// is alignment currently running

	int m_algnPositionIndex{ 0 };

	QTimer *m_algnTimer = nullptr;

	void calculateVoltages(ODT_MODE);

private slots:
	void acquire(std::unique_ptr <StorageWrapper> & storage) override;
	void nextAlgnPosition();

signals:
	void s_acqSettingsChanged(ODT_SETTINGS);				// emit the acquisition voltages
	void s_algnRunning(bool);								// is alignment running
	void s_algnSettingsChanged(ODT_SETTINGS);				// emit the alignment voltages
	void s_mirrorVoltageChanged(VOLTAGE2, ODT_MODE);		// emit the mirror voltage
};

#endif //ODT_H