#include "stdafx.h"
#include "scancontrol.h"

bool ScanControl::getConnectionStatus() {
	return m_isConnected + m_isCompatible;
}

void ScanControl::movePosition(POINT3 distance) {
	POINT3 position = getPosition() + distance;
	setPosition(position);
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

void ScanControl::startAnnouncingElementPosition() {
	if (elementPositionTimer) {
		elementPositionTimer->start(100);
	}
};

void ScanControl::stopAnnouncingElementPosition() {
	if (elementPositionTimer) {
		elementPositionTimer->stop();
	}
};

void ScanControl::setHome() {
	m_homePosition = getPosition();
	announceSavedPositionsNormalized();
	announcePosition();
	calculateHomePositionBounds();
}

void ScanControl::calculateHomePositionBounds() {
	m_homePositionBounds.xMin = m_absoluteBounds.xMin - m_homePosition.x;
	m_homePositionBounds.xMax = m_absoluteBounds.xMax - m_homePosition.x;
	m_homePositionBounds.yMin = m_absoluteBounds.yMin - m_homePosition.y;
	m_homePositionBounds.yMax = m_absoluteBounds.yMax - m_homePosition.y;
	m_homePositionBounds.zMin = m_absoluteBounds.zMin - m_homePosition.z;
	m_homePositionBounds.zMax = m_absoluteBounds.zMax - m_homePosition.z;

	emit(homePositionBoundsChanged(m_homePositionBounds));
}

void ScanControl::calculateCurrentPositionBounds() {
	POINT3 currentPosition = getPosition();
	m_currentPositionBounds.xMin = m_absoluteBounds.xMin - currentPosition.x;
	m_currentPositionBounds.xMax = m_absoluteBounds.xMax - currentPosition.x;
	m_currentPositionBounds.yMin = m_absoluteBounds.yMin - currentPosition.y;
	m_currentPositionBounds.yMax = m_absoluteBounds.yMax - currentPosition.y;
	m_currentPositionBounds.zMin = m_absoluteBounds.zMin - currentPosition.z;
	m_currentPositionBounds.zMax = m_absoluteBounds.zMax - currentPosition.z;

	emit(currentPositionBoundsChanged(m_currentPositionBounds));
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
