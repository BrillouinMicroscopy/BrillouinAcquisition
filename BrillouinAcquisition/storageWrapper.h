#ifndef STORAGEWRAPPER_H
#define STORAGEWRAPPER_H

#include "external/h5bm/h5bm.h"
#include "andor.h"

struct IMAGE {
public:
	IMAGE(int indX, int indY, int indZ, int rank, hsize_t *dims, std::string date, std::vector<unsigned short> data) :
		indX(indX), indY(indY), indZ(indZ), rank(rank), dims(dims), date(date), data(data) {};

	const int indX;
	const int indY;
	const int indZ;
	const int rank;
	const hsize_t *dims;
	const std::string date;
	const std::vector<unsigned short> data;
};

struct CALIBRATION {
public:
	CALIBRATION(int index, std::vector<unsigned short> data, int rank, hsize_t *dims, std::string sample, double shift, std::string date) :
		index(index), data(data), rank(rank), dims(dims), sample(sample), shift(shift), date(date) {};

	const int index;
	const std::vector<unsigned short> data;
	const int rank;
	const hsize_t *dims;
	const std::string sample;
	const double shift;
	const std::string date;
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
		const std::string filename = "Brillouin.h5",
		int flags = H5F_ACC_RDONLY
	) : H5BM(parent, filename, flags) {};
	~StorageWrapper();

	QQueue<IMAGE*> m_payloadQueue;
	QQueue<CALIBRATION*> m_calibrationQueue;
	bool m_abort = false;

	void startWritingQueues();
	void stopWritingQueues();

	int m_writtenImagesNr = 0;
	int m_writtenCalibrationsNr = 0;

public slots:
	void s_writeQueues();

	void s_enqueuePayload(IMAGE*);
	void s_enqueueCalibration(CALIBRATION *cal);

	void s_finishedQueueing();

signals:
	void finished();
};

#endif //STORAGEWRAPPER_H
