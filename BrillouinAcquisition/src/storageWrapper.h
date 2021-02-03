#ifndef STORAGEWRAPPER_H
#define STORAGEWRAPPER_H

#include "../external/h5bm/h5bm.h"

class StoragePath {
public:
	StoragePath() {};
	StoragePath(const std::string& filename, const std::string& folder) : filename(filename), folder(folder) {};

	std::string filename{ "Brillouin.h5" };	// filename
	std::string folder{ "." };

	std::string fullPath() {
		return folder + '/' + filename;
	}
};

class StorageWrapper : public H5BM {
	Q_OBJECT

public:
	StorageWrapper(
		QObject *parent = nullptr,
		const std::string& fullPath = StoragePath{}.fullPath(),//"./Brillouin.h5",
		int flags = H5F_ACC_RDONLY
	) noexcept : H5BM(parent, fullPath, flags) {};
	~StorageWrapper();

	QQueue<IMAGE<unsigned char>*> m_payloadQueueBrillouin_char;
	QQueue<IMAGE<unsigned short>*> m_payloadQueueBrillouin_short;
	QQueue<IMAGE<unsigned int>*> m_payloadQueueBrillouin_int;

	QQueue<ODTIMAGE<unsigned char>*> m_payloadQueueODT_char;
	QQueue<ODTIMAGE<unsigned short>*> m_payloadQueueODT_short;

	QQueue<FLUOIMAGE<unsigned char>*> m_payloadQueueFluorescence_char;
	QQueue<FLUOIMAGE<unsigned short>*> m_payloadQueueFluorescence_short;

	QQueue<CALIBRATION<unsigned char>*> m_calibrationQueue_char;
	QQueue<CALIBRATION<unsigned short>*> m_calibrationQueue_short;
	QQueue<CALIBRATION<unsigned int>*> m_calibrationQueue_int;

	bool m_abort{ false };

	int m_writtenImagesNr{ 0 };
	int m_writtenCalibrationsNr{ 0 };

public slots:
	void init();

	void startWritingQueues();
	void stopWritingQueues();

	void s_writeQueues();

	void s_enqueuePayload(IMAGE<unsigned char>*);
	void s_enqueuePayload(IMAGE<unsigned short>*);
	void s_enqueuePayload(IMAGE<unsigned int>*);

	void s_enqueuePayload(ODTIMAGE<unsigned char>*);
	void s_enqueuePayload(ODTIMAGE<unsigned short>*);

	void s_enqueuePayload(FLUOIMAGE<unsigned char>*);
	void s_enqueuePayload(FLUOIMAGE<unsigned short>*);

	void s_enqueueCalibration(CALIBRATION<unsigned char>* cal);
	void s_enqueueCalibration(CALIBRATION<unsigned short>* cal);
	void s_enqueueCalibration(CALIBRATION<unsigned int>* cal);

	void s_finishedQueueing();

private:
	bool m_finished{ false };
	bool m_observeQueues{ false };
	bool m_finishedQueueing{ false };

	QTimer* m_queueTimer{ nullptr };

signals:
	void finished();
	void started();
};

#endif //STORAGEWRAPPER_H
