#ifndef ACQUISITION_H
#define ACQUISITION_H

#include "../wrapper/storage.h"
#include "../helper/thread.h"

struct REPETITIONS {
	int count{ 1 };			// [1]		number of repetitions
	double interval{ 10 };	// [min]	interval between repetitions
	bool filePerRepetition{ false };	// [bool]	create a new file per repetition
};

class Acquisition : public QObject {
	Q_OBJECT

public:
	explicit Acquisition(QObject *parent);
	~Acquisition();
	ACQUISITION_MODE getEnabledModes();
	std::unique_ptr <StorageWrapper> m_storage{ nullptr };

public slots:
	void init() {};
	// overrides previous acquisition (potential dataloss)
	void newFile(StoragePath path);
	/*
	 * Opens acquisition and adds new repetitions without overriding by default.
	 * Different behaviour can be specified by supplying a different flag.
	 */
	void openFile(const StoragePath& path, int flag = H5F_ACC_RDWR, bool forceOpen = false);
	void openFile(std::string filename = "", bool forceOpen = false);
	void newRepetition(ACQUISITION_MODE mode);
	void startedWritingToFile();
	void finishedWritingToFile();
	int closeFile();

	std::string getCurrentFolder();

	std::string getCurrentFilename();
	
	bool isModeEnabled(ACQUISITION_MODE mode);

	bool enableMode(ACQUISITION_MODE);
	void disableMode(ACQUISITION_MODE);

private:
	StoragePath m_path;
	ACQUISITION_MODE m_enabledModes{ ACQUISITION_MODE::NONE };	// which mode is currently acquiring
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