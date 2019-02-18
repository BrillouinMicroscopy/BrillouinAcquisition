#ifndef STORAGEWRAPPER_H
#define STORAGEWRAPPER_H

#include "external/h5bm/h5bm.h"

class StoragePath {
public:
	StoragePath() {};
	StoragePath(const std::string filename, const std::string folder) : filename(filename), folder(folder) {};

	std::string filename = "Brillouin.h5";	// filename
	std::string folder = ".";

	std::string fullPath() {
		return folder + '/' + filename;
	}
};

class StorageWrapper : public H5BM {
	Q_OBJECT
private:
	bool m_finished = false;
	bool m_observeQueues = false;
	bool m_finishedQueueing = false;

	QTimer *queueTimer = nullptr;

public:
	StorageWrapper(
		QObject *parent = nullptr,
		const std::string fullPath = StoragePath{}.fullPath(),//"./Brillouin.h5",
		int flags = H5F_ACC_RDONLY
	) noexcept : H5BM(parent, fullPath, flags) {};
	~StorageWrapper();

	QQueue<IMAGE*> m_payloadQueueBrillouin;
	QQueue<ODTIMAGE*> m_payloadQueueODT;
	QQueue<FLUOIMAGE*> m_payloadQueueFluorescence;
	QQueue<CALIBRATION*> m_calibrationQueue;
	bool m_abort = false;

	int m_writtenImagesNr{ 0 };
	int m_writtenCalibrationsNr{ 0 };

public slots:
	void init();

	void startWritingQueues();
	void stopWritingQueues();

	void s_writeQueues();

	void s_enqueuePayload(IMAGE*);
	void s_enqueuePayload(ODTIMAGE*);
	void s_enqueuePayload(FLUOIMAGE*);
	void s_enqueueCalibration(CALIBRATION *cal);

	void s_finishedQueueing();

signals:
	void finished();
	void started();
};

#endif //STORAGEWRAPPER_H
