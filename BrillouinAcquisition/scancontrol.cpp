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
