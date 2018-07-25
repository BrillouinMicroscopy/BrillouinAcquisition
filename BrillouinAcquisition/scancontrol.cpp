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
}

void ScanControl::moveHome() {
	setPosition(m_homePosition);
}

void ScanControl::savePosition() {
	POINT3 position = getPosition();
	m_savedPositions.push_back(position);
	emit(savedPositionsChanged(m_savedPositions));
}

void ScanControl::moveToSavedPosition(int index) {
	if (m_savedPositions.size() > index) {
		setPosition(m_savedPositions[index]);
	}
}

void ScanControl::deleteSavedPosition(int index) {
	if (m_savedPositions.size() > index) {
		m_savedPositions.erase(m_savedPositions.begin() + index);
		emit(savedPositionsChanged(m_savedPositions));
	}
}
