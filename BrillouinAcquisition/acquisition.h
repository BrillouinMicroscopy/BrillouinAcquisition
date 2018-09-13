#ifndef ACQUISITION_H
#define ACQUISITION_H

#include "storageWrapper.h"
#include "thread.h"

enum class ACQUISITION_STATE {
	STARTED,
	RUNNING,
	FINISHED,
	ABORTED
};

struct ACQUISITION_STATES {
	ACQUISITION_STATE Brillouin = ACQUISITION_STATE::FINISHED;
	ACQUISITION_STATE ODT = ACQUISITION_STATE::FINISHED;
	ACQUISITION_STATE Fluorescence = ACQUISITION_STATE::FINISHED;
};

struct REPETITIONS {
	int count = 1;			// [1]		number of repetitions
	double interval = 10;	// [min]	interval between repetitions
};

class Acquisition : public QObject {
	Q_OBJECT

public:
	Acquisition(QObject *parent);
	~Acquisition();
	ACQUISITION_MODE isAcqRunning();
	std::unique_ptr <StorageWrapper> m_storage = nullptr;

public slots:
	void init() {};
	void newAcquisition(StoragePath path);
	void newRepetition(ACQUISITION_MODE mode);
	void setAcquisitionMode(ACQUISITION_MODE mode);
	void setAcquisitionState(ACQUISITION_MODE mode, ACQUISITION_STATE state);

private:
	StoragePath m_path;
	ACQUISITION_MODE m_modeRunning = ACQUISITION_MODE::NONE;	// which mode is currently acquiring
	ACQUISITION_STATES m_states;								// state of the acquisition modes
	void checkFilename();

signals:
	void s_acqModeRunning(ACQUISITION_MODE);	// which acquisition mode is running
	void s_filenameChanged(std::string);
};

#endif //ACQUISITION_H