#include "stdafx.h"
#include "AcquisitionMode.h"

/*
 * Public definitions
 */

AcquisitionMode::AcquisitionMode(QObject *parent, Acquisition *acquisition)
	: QObject(parent), m_acquisition(acquisition) {
}

AcquisitionMode::~AcquisitionMode() {
}

/*
 * Public slots
 */

ACQUISITION_STATUS AcquisitionMode::getStatus() {
	return m_status;
}

/*
 * Protected definitions
 */

void AcquisitionMode::setAcquisitionStatus(ACQUISITION_STATUS status) {
	m_status = status;
	emit(s_acquisitionStatus(m_status));
}
