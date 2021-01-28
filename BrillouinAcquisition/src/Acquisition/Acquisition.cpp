#include "stdafx.h"
#include "acquisition.h"
#include "../simplemath.h"
#include "../logger.h"
#include "filesystem"

using namespace std::filesystem;


Acquisition::Acquisition(QObject *parent)
	: QObject(parent) {
	// Create a new thread for backgroud storage
	m_storageThread = new Thread();
}

Acquisition::~Acquisition() {
	m_enabledModes = ACQUISITION_MODE::NONE;
	// Wait for storage thread to finish writing the queue
	m_storage.reset(nullptr);
	m_storageThread->exit();
	m_storageThread->wait();
}

ACQUISITION_MODE Acquisition::getEnabledModes() {
	return m_enabledModes;
}

void Acquisition::newFile(StoragePath path) {
	openFile(path, H5F_ACC_TRUNC);
}

void Acquisition::openFile(StoragePath path, int flag, bool forceOpen) {
	// if an acquisition is running, do nothing
	if (m_enabledModes != ACQUISITION_MODE::NONE && !forceOpen) {
		emit(s_enabledModes(m_enabledModes));
		return;
	}
	m_path = path;
	
	emit(s_filenameChanged(m_path.filename));
	m_storage = std::make_unique <StorageWrapper>(nullptr, m_path.fullPath(), flag);

	// Move storage object to own thread
	m_storageThread->startWorker(m_storage.get());

	auto connection = QWidget::connect(
		m_storage.get(),
		&StorageWrapper::started,
		this,
		&Acquisition::startedWritingToFile
	);
	connection = QWidget::connect(
		m_storage.get(),
		&StorageWrapper::finished,
		this,
		&Acquisition::finishedWritingToFile
	);
}

void Acquisition::openFile(std::string filename, bool forceOpen) {
	if (filename.empty()) {
		m_path.filename = StoragePath{}.filename;
	} else {
		m_path.filename = filename;
	}
	StoragePath defaultPath = checkFilename(m_path);

	openFile(defaultPath, H5F_ACC_RDWR, forceOpen);
}

void Acquisition::newRepetition(ACQUISITION_MODE mode) {
	if (m_storage == nullptr) {
		openFile();
	}
	m_storage->newRepetition(mode);
}

void Acquisition::startedWritingToFile() {
	m_writingToFile = true;
}

void Acquisition::finishedWritingToFile() {
	m_writingToFile = false;
}

int Acquisition::closeFile() {
	// if an acquisition is running, do nothing
	if (m_enabledModes != ACQUISITION_MODE::NONE || m_writingToFile) {
		emit(s_enabledModes(m_enabledModes));
		return -1;
	}
	if (m_storage) {
		// Create a reference to the current thread
		QThread* acquisitionThread = QThread::currentThread();
		// Move the acquisition class back to the main thread
		m_storage->moveToThread(acquisitionThread);
		m_storage.reset();
	}
	return 0;
}

std::string Acquisition::getCurrentFolder() {
	return m_path.folder;
}

std::string Acquisition::getCurrentFilename() {
	return m_path.filename;
}

bool Acquisition::isModeEnabled(ACQUISITION_MODE mode) {
	return (bool)(m_enabledModes & mode);
}

/*
 * Function checks if starting an acquisition of given mode is allowed.
 * If yes, it adds the requested mode to the currently running acquisition to modes.
 */
bool Acquisition::enableMode(ACQUISITION_MODE mode) {
	// If no acquisition file is open, open one.
	if (m_storage == nullptr && mode != ACQUISITION_MODE::VOLTAGECALIBRATION) {
		openFile();
	}

	// Check that the requested mode is not already running.
	if ((bool)(mode& m_enabledModes)) {
		return true;
	}
	// Check, that no other mode is enabled when spatial calibration is requested
	if (mode == ACQUISITION_MODE::VOLTAGECALIBRATION && m_enabledModes != ACQUISITION_MODE::NONE) {
		return false;
	}
	// Check, that Brillouin and ODT don't run simultaneously.
	if (((mode | m_enabledModes) & (ACQUISITION_MODE::BRILLOUIN | ACQUISITION_MODE::ODT))
		== (ACQUISITION_MODE::BRILLOUIN | ACQUISITION_MODE::ODT)) {
		return false;
	}
	// Check, that Fluorescence and ODT don't run simultaneously.
	if (((mode | m_enabledModes) & (ACQUISITION_MODE::FLUORESCENCE | ACQUISITION_MODE::ODT))
		== (ACQUISITION_MODE::FLUORESCENCE | ACQUISITION_MODE::ODT)) {
		return false;
	}
	// Add the requested mode to the running modes.
	m_enabledModes |= mode;
	emit(s_enabledModes(m_enabledModes));
	return true;
}

/* 
 * Stops the selected mode.
 */
void Acquisition::disableMode(ACQUISITION_MODE mode) {
	m_enabledModes &= ~mode;
	emit(s_enabledModes(m_enabledModes));
}

void Acquisition::checkFilename() {
	m_path = checkFilename(m_path);
}

StoragePath Acquisition::checkFilename(StoragePath desiredPath) {
	std::string oldFilename = desiredPath.filename;
	// get filename without extension
	std::string rawFilename = oldFilename.substr(0, oldFilename.find_last_of("."));
	// remove possibly attached number separated by a hyphen
	rawFilename = rawFilename.substr(0, rawFilename.find_last_of("-"));
	int count = 0;
	while (exists(desiredPath.fullPath())) {
		desiredPath.filename = rawFilename + '-' + std::to_string(count) + oldFilename.substr(oldFilename.find_last_of("."), std::string::npos);
		count++;
	}
	if (desiredPath.filename != oldFilename) {
		emit(s_filenameChanged(desiredPath.filename));
	}
	return desiredPath;
}