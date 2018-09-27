#ifndef STORAGEWRAPPER_H
#define STORAGEWRAPPER_H

#include "external/h5bm/h5bm.h"

struct StoragePath {
	std::string filename = "Brillouin.h5";	// filename
	std::string folder = ".";
	std::string fullPath = folder + '/' + filename;
};

class StorageWrapper : public H5BM {
	Q_OBJECT
private:
	bool m_finished = false;
	bool m_observeQueues = false;
	bool m_finishedQueueing = false;

public:
	StorageWrapper(
		QObject *parent = nullptr,
		const StoragePath path = {},
		int flags = H5F_ACC_RDONLY
	) noexcept : H5BM(parent, path.fullPath, flags) {};
	~StorageWrapper();

	QQueue<IMAGE*> m_payloadQueueBrillouin;
	QQueue<ODTIMAGE*> m_payloadQueueODT;
	QQueue<CALIBRATION*> m_calibrationQueue;
	bool m_abort = false;

	void startWritingQueues();
	void stopWritingQueues();

	int m_writtenImagesNr{ 0 };
	int m_writtenCalibrationsNr{ 0 };

public slots:
	void s_writeQueues();

	void s_enqueuePayload(IMAGE*);
	void s_enqueuePayload(ODTIMAGE*);
	void s_enqueueCalibration(CALIBRATION *cal);

	void s_finishedQueueing();

signals:
	void finished();
};

#endif //STORAGEWRAPPER_H
