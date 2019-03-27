#ifndef ACQUISITIONMODE_H
#define ACQUISITIONMODE_H

#include <QtCore>
#include <gsl/gsl>

#include "../Acquisition.h"

enum class ACQUISITION_STATUS {
	DISABLED,
	ENABLED,
	ABORTED,
	FINISHED,
	STOPPED,
	ALIGNING,
	STARTED,
	RUNNING
};

class AcquisitionMode : public QObject {
	Q_OBJECT

public:
	AcquisitionMode(QObject *parent, Acquisition *acquisition);
	~AcquisitionMode();
	bool m_abort = false;

public slots:
	void init() {};
	virtual void startRepetitions() = 0;
	ACQUISITION_STATUS getStatus();

protected:
	Acquisition *m_acquisition = nullptr;
	virtual void abortMode(std::unique_ptr <StorageWrapper> & storage) = 0;
	ACQUISITION_STATUS m_status{ ACQUISITION_STATUS::DISABLED };

private slots:
	virtual void acquire(std::unique_ptr <StorageWrapper> & storage) = 0;

signals:
	void s_acquisitionStatus(ACQUISITION_STATUS);	// current acquisition state
	void s_repetitionProgress(double, int);		// progress in percent and the remaining time in seconds
	void s_totalProgress(int, int);				// repetitions
};

#endif //ACQUISITIONMODE_H