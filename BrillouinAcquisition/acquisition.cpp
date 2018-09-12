#include "stdafx.h"
#include "acquisition.h"
#include "simplemath.h"
#include "logger.h"
#include "filesystem"

using namespace std::experimental::filesystem::v1;


Acquisition::Acquisition(QObject *parent)
	: QObject(parent) {
}

Acquisition::~Acquisition() {
	m_modeRunning = ACQUISITION_MODE::NONE;
}

ACQUISITION_MODE Acquisition::isAcqRunning() {
	return m_modeRunning;
}

void Acquisition::newAcquisition(StoragePath path) {
	// if an acquisition is running, do nothing
	if (m_modeRunning != ACQUISITION_MODE::NONE) {
		emit(s_acqModeRunning(m_modeRunning));
		return;
	}
	m_path = path;

	checkFilename();
	
	m_storage = std::make_unique <StorageWrapper>(nullptr, m_path, H5F_ACC_RDWR);
}

void Acquisition::setAcquisitionMode(ACQUISITION_MODE mode) {
	m_modeRunning = mode;
	emit(s_acqModeRunning(m_modeRunning));
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

void Acquisition::checkFilename() {
	std::string oldFilename = m_path.filename;
	// get filename without extension
	std::string rawFilename = oldFilename.substr(0, oldFilename.find_last_of("."));
	// remove possibly attached number separated by a hyphen
	rawFilename = rawFilename.substr(0, rawFilename.find_last_of("-"));
	m_path.fullPath = m_path.folder + "/" + m_path.filename;
	int count = 0;
	while (exists(m_path.fullPath)) {
		m_path.filename = rawFilename + '-' + std::to_string(count) + oldFilename.substr(oldFilename.find_last_of("."), std::string::npos);
		m_path.fullPath = m_path.folder + "/" + m_path.filename;
		count++;
	}
	if (m_path.filename != oldFilename) {
		emit(s_filenameChanged(m_path.filename));
	}
}
