#include "stdafx.h"
#include "scancontrol.h"

bool ScanControl::getConnectionStatus() {
	return m_isConnected + m_isCompatible;
}

void ScanControl::announcePosition() {
	POINT3 point = getPosition();
	emit(currentPosition(point - m_homePosition));
}

void ScanControl::startAnnouncingPosition() {
	if (positionTimer) {
		positionTimer->start(100);
	}
};

void ScanControl::stopAnnouncingPosition() {
	if (positionTimer) {
		positionTimer->stop();
	}
};

void ScanControl::setHome() {
	m_homePosition = getPosition();
	announceSavedPositionsNormalized();
}

void ScanControl::moveHome() {
	setPosition(m_homePosition);
}

void ScanControl::savePosition() {
	POINT3 position = getPosition();
	m_savedPositions.push_back(position);
	announceSavedPositionsNormalized();
}

void ScanControl::moveToSavedPosition(int index) {
	if (m_savedPositions.size() > index) {
		setPosition(m_savedPositions[index]);
	}
}

void ScanControl::deleteSavedPosition(int index) {
	if (m_savedPositions.size() > index) {
		m_savedPositions.erase(m_savedPositions.begin() + index);
		announceSavedPositionsNormalized();
	}
}

std::vector<POINT3> ScanControl::getSavedPositionsNormalized() {
	std::vector<POINT3> savedPositionsNormalized = m_savedPositions;
	std::transform(savedPositionsNormalized.begin(), savedPositionsNormalized.end(), savedPositionsNormalized.begin(),
		[this](POINT3 point) {
			return point - this->m_homePosition;
		}
	);
	return savedPositionsNormalized;
}

void ScanControl::announceSavedPositionsNormalized() {
	std::vector<POINT3> savedPositionsNormalized = getSavedPositionsNormalized();
	emit(savedPositionsChanged(savedPositionsNormalized));
}
