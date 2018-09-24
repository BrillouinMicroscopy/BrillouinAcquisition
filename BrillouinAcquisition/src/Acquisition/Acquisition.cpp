#include "stdafx.h"
#include "acquisition.h"
#include "../simplemath.h"
#include "../logger.h"
#include "filesystem"

using namespace std::experimental::filesystem::v1;


Acquisition::Acquisition(QObject *parent)
	: QObject(parent) {
}

Acquisition::~Acquisition() {
	m_currentModes = ACQUISITION_MODE::NONE;
}

ACQUISITION_MODE Acquisition::getCurrentModes() {
	return m_currentModes;
}

void Acquisition::newFile(StoragePath path) {
	openFile(path, H5F_ACC_TRUNC);
}

void Acquisition::openFile(StoragePath path, int flag) {
	// if an acquisition is running, do nothing
	if (m_currentModes != ACQUISITION_MODE::NONE) {
		emit(s_currentModes(m_currentModes));
		return;
	}
	m_path = path;
	
	emit(s_filenameChanged(m_path.filename));
	m_storage = std::make_unique <StorageWrapper>(nullptr, m_path, flag);
}

void Acquisition::openFile() {
	StoragePath defaultPath = StoragePath{};
	defaultPath = checkFilename(defaultPath);

	openFile(defaultPath);
}

void Acquisition::newRepetition(ACQUISITION_MODE mode) {
	if (m_storage == nullptr) {
		openFile();
	}
	m_storage->newRepetition(mode);
}

void Acquisition::closeFile() {
	m_storage.reset();
}

void Acquisition::setAcquisitionState(ACQUISITION_MODE mode, ACQUISITION_STATE state) {
	switch (mode) {
		case ACQUISITION_MODE::BRILLOUIN:
			m_states.Brillouin = state;
			break;
		case ACQUISITION_MODE::ODT:
			m_states.ODT = state;
			break;
		case ACQUISITION_MODE::FLUORESCENCE:
			m_states.Fluorescence = state;
			break;
		default:
			break;
	}
}

bool Acquisition::isModeEnabled(ACQUISITION_MODE mode) {
	return (bool)(m_currentModes & mode);
}

/*
 * Function checks if starting an acquisition of given mode is allowed.
 * If yes, it adds the requested mode to the currently running acquisition to modes.
 */
bool Acquisition::enableMode(ACQUISITION_MODE mode) {
	// If no acquisition file is open, open one.
	if (m_storage == nullptr) {
		openFile();
	}

	// Check that the requested mode is not already running.
	if ((bool)(mode & m_currentModes)) {
		return true;
	}
	// Check, that Brillouin and ODT don't run simultaneously.
	if (((mode | m_currentModes) & (ACQUISITION_MODE::BRILLOUIN | ACQUISITION_MODE::ODT))
		== (ACQUISITION_MODE::BRILLOUIN | ACQUISITION_MODE::ODT)) {
		return false;
	}
	// Check, that Fluorescence and ODT don't run simultaneously.
	if (((mode | m_currentModes) & (ACQUISITION_MODE::FLUORESCENCE | ACQUISITION_MODE::ODT))
		== (ACQUISITION_MODE::FLUORESCENCE | ACQUISITION_MODE::ODT)) {
		return false;
	}
	// Add the requested mode to the running modes.
	m_currentModes |= mode;
	emit(s_currentModes(m_currentModes));
	return true;
}

/* 
 * Stops the selected mode.
 */
void Acquisition::disableMode(ACQUISITION_MODE mode) {
	m_currentModes &= ~mode;
	emit(s_currentModes(m_currentModes));
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
	desiredPath.fullPath = desiredPath.folder + "/" + desiredPath.filename;
	int count = 0;
	while (exists(desiredPath.fullPath)) {
		desiredPath.filename = rawFilename + '-' + std::to_string(count) + oldFilename.substr(oldFilename.find_last_of("."), std::string::npos);
		desiredPath.fullPath = desiredPath.folder + "/" + desiredPath.filename;
		count++;
	}
	if (desiredPath.filename != oldFilename) {
		emit(s_filenameChanged(desiredPath.filename));
	}
	return desiredPath;
}