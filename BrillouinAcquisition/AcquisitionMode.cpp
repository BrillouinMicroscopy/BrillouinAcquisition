#include "stdafx.h"
#include "AcquisitionMode.h"

AcquisitionMode::AcquisitionMode(QObject *parent, Acquisition *acquisition)
	: QObject(parent), m_acquisition(acquisition) {
}

AcquisitionMode::~AcquisitionMode() {
	m_running = false;
}

bool AcquisitionMode::isRunning() {
	return m_running;
}

void AcquisitionMode::abort() {
	m_abort = true;
}