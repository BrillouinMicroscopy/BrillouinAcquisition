#ifndef ACQUISITIONMODE_H
#define ACQUISITIONMODE_H

#include <QtCore>
#include <gsl/gsl>

#include "..\acquisition.h"

class AcquisitionMode : public QObject {
	Q_OBJECT

public:
	AcquisitionMode(QObject *parent, Acquisition *acquisition);
	~AcquisitionMode();
	bool m_abort = false;
	virtual void abort();

public slots:
	void init() {};
	virtual void startRepetitions() = 0;

protected:
	Acquisition *m_acquisition = nullptr;

private slots:
	virtual void acquire(std::unique_ptr <StorageWrapper> & storage) = 0;

signals:
	void s_repetitionProgress(ACQUISITION_STATE, double, int);	// progress in percent and the remaining time in seconds
	void s_totalProgress(int, int);								// repetitions
};

#endif //ACQUISITIONMODE_H