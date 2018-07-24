#include "stdafx.h"
#include "scancontrol.h"

bool ScanControl::getConnectionStatus() {
	return m_isConnected + m_isCompatible;
}

void ScanControl::announcePosition() {
	POINT3 point = getPosition();
	point.x -= m_homePosition.x;
	point.y -= m_homePosition.y;
	point.z -= m_homePosition.z;
	emit(currentPosition(point));
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