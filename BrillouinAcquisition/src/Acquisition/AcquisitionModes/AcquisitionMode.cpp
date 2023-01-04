#include "stdafx.h"
#include "AcquisitionMode.h"

/*
 * Public definitions
 */

AcquisitionMode::AcquisitionMode(QObject *parent, Acquisition *acquisition, ScanControl*& scanControl)
	: QObject(parent), m_acquisition(acquisition), m_scanControl(scanControl) {
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

void AcquisitionMode::writeScaleCalibration(std::unique_ptr <StorageWrapper>& storage, ACQUISITION_MODE mode) {
	auto scaleCalibration = m_scanControl->getScaleCalibration();

	auto positionStage = m_scanControl->getPosition(PositionType::STAGE);
	auto positionScanner = m_scanControl->getPosition(PositionType::SCANNER);

	storage->setScaleCalibration(mode, { scaleCalibration, positionStage, positionScanner });
}
