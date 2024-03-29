#ifndef ACQUISITIONMODE_H
#define ACQUISITIONMODE_H

#include <QtCore>
#include <gsl/gsl>

#include "..\Acquisition.h"
#include "..\..\Devices\ScanControls\ScanControl.h"

enum class ACQUISITION_STATUS {
	DISABLED,
	ENABLED,
	ABORTED,
	FINISHED,
	STOPPED,
	ALIGNING,
	WAITFORREPETITION,
	STARTED,
	RUNNING
};

class AcquisitionMode : public QObject {
	Q_OBJECT

public:
	AcquisitionMode(QObject *parent, Acquisition* acquisition, ScanControl*& scanControl);
	~AcquisitionMode();

	bool m_abort{ false };

public slots:
	virtual void startRepetitions() = 0;

	virtual void init() {};

	ACQUISITION_STATUS getStatus();

protected:
	virtual void abortMode(std::unique_ptr <StorageWrapper> & storage) = 0;

	void setAcquisitionStatus(ACQUISITION_STATUS);

	void writeScaleCalibration(std::unique_ptr <StorageWrapper>& storage, ACQUISITION_MODE mode);

	ACQUISITION_STATUS m_status{ ACQUISITION_STATUS::DISABLED };
	Acquisition* m_acquisition{ nullptr };
	ScanControl*& m_scanControl;

private slots:
	virtual void acquire(std::unique_ptr <StorageWrapper> & storage) = 0;

signals:
	void s_acquisitionStatus(ACQUISITION_STATUS);	// current acquisition state
	void s_repetitionProgress(double, int);		// progress in percent and the remaining time in seconds
	void s_totalProgress(int, int);				// repetitions
};

#endif //ACQUISITIONMODE_H