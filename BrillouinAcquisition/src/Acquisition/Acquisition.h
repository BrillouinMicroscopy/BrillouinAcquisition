#ifndef ACQUISITION_H
#define ACQUISITION_H

#include "../storageWrapper.h"
#include "../thread.h"

struct REPETITIONS {
	int count = 1;			// [1]		number of repetitions
	double interval = 10;	// [min]	interval between repetitions
};

class Acquisition : public QObject {
	Q_OBJECT

public:
	Acquisition(QObject *parent);
	~Acquisition();
	ACQUISITION_MODE getEnabledModes();
	std::unique_ptr <StorageWrapper> m_storage = nullptr;

public slots:
	void init() {};
	// overrides previous acquisition (potential dataloss)
	void newFile(StoragePath path);
	/*
	 * Opens acquisition and adds new repetitions without overriding by default.
	 * Different behaviour can be specified by supplying a different flag.
	 */
	void openFile(StoragePath path, int flag = H5F_ACC_RDWR);
	void openFile();
	void newRepetition(ACQUISITION_MODE mode);
	void startedWritingToFile();
	void finishedWritingToFile();
	int closeFile();

	std::string getCurrentFolder();
	
	bool isModeEnabled(ACQUISITION_MODE mode);

	bool enableMode(ACQUISITION_MODE);
	void disableMode(ACQUISITION_MODE);

private:
	StoragePath m_path;
	ACQUISITION_MODE m_enabledModes = ACQUISITION_MODE::NONE;	// which mode is currently acquiring
	Thread* m_storageThread;
	bool m_writingToFile{ false };

private slots:
	void checkFilename();
	StoragePath checkFilename(StoragePath desiredPath);

signals:
	void s_enabledModes(ACQUISITION_MODE);	// which acquisition mode is running
	void s_filenameChanged(std::string);
	void s_openFileFailed();
};

#endif //ACQUISITION_H